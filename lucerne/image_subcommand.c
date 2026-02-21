//
//  image_subcommand.c
//  lucerne
//
//  Created by Antoine on 21/02/2026.
//  

#include "image_subcommand.h"
#include "target.h"
#include <stdio.h>
#include <mach-o/dyld_images.h>
#include <stdlib.h>

// code in memory_read_subcommand already sort of does this,
// but that is specialized for the memory subcommand (ie, shows errors specific to that)
// so we just implement a very simple mach_vm_read_overwrite wrapper here
static kern_return_t read_remote_simple(task_t task,
                                        mach_vm_address_t addr,
                                        void *buf,
                                        size_t size) {
    mach_vm_size_t outSize = 0;
    return mach_vm_read_overwrite(task, addr,
                                  (mach_vm_size_t)size, (mach_vm_address_t)buf,
                                  &outSize);
}

static kern_return_t read_remote_cstr(task_t task,
                                      mach_vm_address_t addr,
                                      char *out,
                                      size_t outCap) {
    // Read in small chunks until NULL or cap-1.
    size_t written = 0;
    while (written + 1 < outCap) {
        char chunk[256];
        size_t want = sizeof(chunk);
        if (written + want >= outCap) want = outCap - 1 - written;

        mach_vm_size_t outSize = 0;
        kern_return_t kr = mach_vm_read_overwrite(
            task,
            addr + written,
            (mach_vm_size_t)want,
            (mach_vm_address_t)chunk,
            &outSize
        );
        
        if (kr != KERN_SUCCESS)
            return kr;

        for (mach_vm_size_t i = 0; i < outSize && written + 1 < outCap; i++) {
            out[written++] = chunk[i];
            if (chunk[i] == '\0') return KERN_SUCCESS;
        }
    }
    out[outCap - 1] = '\0';
    return KERN_SUCCESS;
}

void image_subcommand(int argc, char **argv) {
}

struct hack_task_dyld_info {
  mach_vm_address_t all_image_info_addr;
  mach_vm_size_t all_image_info_size;
};

void image_list_subcommand(int argc, char **argv) {
    if (get_connected_target()) {
        // first, we need to get the addresses of the
        struct hack_task_dyld_info dyldInfo = {0};
        mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
        
        kern_return_t kr = task_info(get_connected_target()->task,
                                     TASK_DYLD_INFO,
                                     (task_info_t)&dyldInfo,
                                     &count);
        
        if (kr != KERN_SUCCESS) {
            printf("Failed to task_info our target: %s\n", mach_error_string(kr));
            return;
        }
        
        // address of the loaded dyld images *in the remote process*
        // we need to remote read this ourselves... ugh
        mach_vm_address_t allImagesInfoAddr = (mach_vm_address_t)dyldInfo.all_image_info_addr;
        struct dyld_all_image_infos infos;
        kern_return_t readImagesKr = read_remote_simple(get_connected_target()->task,
                                                        allImagesInfoAddr,
                                                        &infos,
                                                        sizeof(infos));
        
        if (readImagesKr != KERN_SUCCESS) {
            printf("Couldn't read address of images in remote process: %s\n",
                   mach_error_string(readImagesKr));
            return;
        }
        
        // unfortunately we cannot read the file paths of the images
        // directly from infos.infoArray
        // (because that then resolves to the debugger's own memory)
        // so we need to remote read that as well
        uint32_t imageCount = infos.infoArrayCount;
        
        size_t arrSize = (size_t)imageCount * sizeof(struct dyld_image_info);
        struct dyld_image_info *actualInfos = malloc(arrSize);
        
        read_remote_simple(get_connected_target()->task,
                           (mach_vm_address_t)infos.infoArray,
                           actualInfos, arrSize);
        
        for (int i = 0; i < imageCount; i++) {
            // print each image in this format (same as LLDB's, conveys enough info):
            // [index] UUID ADDRESS PATH
            mach_vm_address_t filePathAddr = (mach_vm_address_t)actualInfos[i].imageFilePath;
            char path[4096];
            
            read_remote_cstr(get_connected_target()->task, filePathAddr, path, 4096);
            
            printf("[%3d] 0x%016llx %s\n", i,
                   (unsigned long long)actualInfos[i].imageLoadAddress,
                   path);
        }
    }
}
