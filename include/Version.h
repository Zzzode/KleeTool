//
// Created by zode on 2021/2/6.
//

#ifndef TEST_VERSION_H
#define TEST_VERSION_H

#define LLVM_VERSION(major, minor) (((major) << 8) | (minor))
#define LLVM_VERSION_CODE LLVM_VERSION(LLVM_VERSION_MAJOR, LLVM_VERSION_MINOR)

#endif //TEST_VERSION_H
