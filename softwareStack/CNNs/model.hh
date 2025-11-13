/*
 * Copyright EPFL 2023
 * Joshua Klein
 *
 * Header file used to select the model we will use for the CNN application.
 *
 */


#ifndef __MODEL_HH__
#define __MODEL_HH__
#ifdef CNM
#include "cnm.h"
#include "defs.h"
#define NCHANNELS   1
#endif
#if defined (LENET5MNIST)
#include "models/LeNet5MNIST.hh"
#elif defined (ALEXNET)
#include "models/AlexNet.hh"
#elif defined (ALEXNETCIFAR10)
    #if defined (DIANA)
    #include "models/AlexNetCIFAR10_DIANA.hh"
    #elif defined (Q1)
    #include "models/AlexNetCIFAR10_Q1.hh"
    #elif defined (Q2)
    #include "models/AlexNetCIFAR10_Q2.hh"
    #else
    #include "models/AlexNetCIFAR10.hh"
    #endif
#elif defined (ALEXNETCIFAR100)
    #if defined (DIANA)
    #include "models/AlexNetCIFAR100_DIANA.hh"
    #elif defined (Q1)
    #include "models/AlexNetCIFAR100_Q1.hh"
    #elif defined (Q2)
    #include "models/AlexNetCIFAR100_Q2.hh"
    #else
    #include "models/AlexNetCIFAR100.hh"
    #endif
#elif defined (VGG16CIFAR10)
    #if defined (DIANA)
    #include "models/VGG16CIFAR10_DIANA.hh"
    #elif defined (Q1)
    #include "models/VGG16CIFAR10_Q1.hh"
    #elif defined (Q2)
    #include "models/VGG16CIFAR10_Q2.hh"
    #else
    #include "models/VGG16CIFAR10.hh"
    #endif
#elif defined (VGG16CIFAR100)
    #if defined (DIANA)
    #include "models/VGG16CIFAR100_DIANA.hh"
    #elif defined (Q1)
    #include "models/VGG16CIFAR100_Q1.hh"
    #elif defined (Q2)
    #include "models/VGG16CIFAR100_Q2.hh"
    #else
    #include "models/VGG16CIFAR100.hh"
    #endif
#elif defined (RESNET8CIFAR10)
    #if defined (DIANA)
    #include "models/ResNet8CIFAR10_DIANA.hh"
    #elif defined (Q1)
    #include "models/ResNet8CIFAR10_Q1.hh"
    #elif defined (Q2)
    #include "models/ResNet8CIFAR10_Q2.hh"
    #else
    #include "models/ResNet8CIFAR10.hh"
    #endif
#elif defined (RESNET8CIFAR100)
    #if defined (DIANA)
    #include "models/ResNet8CIFAR100_DIANA.hh"
    #elif defined (Q1)
    #include "models/ResNet8CIFAR100_Q1.hh"
    #elif defined (Q2)
    #include "models/ResNet8CIFAR100_Q2.hh"
    #else
    #include "models/ResNet8CIFAR100.hh"
    #endif
#elif defined (RESNET20CIFAR10)
#include "models/ResNet20CIFAR10.hh"
#elif defined (RESNET20CIFAR100)
#include "models/ResNet20CIFAR100.hh"
#elif defined (RESNET27CIFAR10)
    #if defined (DIANA)
    #include "models/ResNet27CIFAR10_DIANA.hh"
    #elif defined (Q1)
    #include "models/ResNet27CIFAR10_Q1.hh"
    #elif defined (Q2)
    #include "models/ResNet27CIFAR10_Q2.hh"
    #else
    #include "models/ResNet27CIFAR10.hh"
    #endif
#elif defined (RESNET27CIFAR100)
    #if defined (DIANA)
    #include "models/ResNet27CIFAR100_DIANA.hh"
    #elif defined (Q1)
    #include "models/ResNet27CIFAR100_Q1.hh"
    #elif defined (Q2)
    #include "models/ResNet27CIFAR100_Q2.hh"
    #else
    #include "models/ResNet27CIFAR100.hh"
    #endif
#elif defined (SSDRESNET34)
#include "models/SSDResNet34.hh"
#elif defined (CHATFIELDF)
#include "models/ChatfieldF.hh"
#elif defined (CHATFIELDM)
#include "models/ChatfieldM.hh"
#elif defined (CHATFIELDS)
#include "models/ChatfieldS.hh"
#elif defined (MOBILENETV2)
#include "models/MobileNetV2.hh"
#elif defined (MOBILENETV2CIFAR10)
#include "models/MobileNetV2CIFAR10.hh"
#elif defined (MOBILENETV2CIFAR100)
#include "models/MobileNetV2CIFAR100.hh"
#elif defined (MOBILENETCIFAR10)
    #if defined (DIANA)
    #include "models/MobileNetCIFAR10_DIANA.hh"
    #elif defined (Q1)
    #include "models/MobileNetCIFAR10_Q1.hh"
    #elif defined (Q2)
    #include "models/MobileNetCIFAR10_Q2.hh"
    #else
    #include "models/MobileNetCIFAR10.hh"
    #endif
#elif defined (MOBILENETCIFAR100)
    #if defined (DIANA)
    #include "models/MobileNetCIFAR100_DIANA.hh"
    #elif defined (Q1)
    #include "models/MobileNetCIFAR100_Q1.hh"
    #elif defined (Q2)
    #include "models/MobileNetCIFAR100_Q2.hh"
    #else
    #include "models/MobileNetCIFAR100.hh"
    #endif
#endif

#endif // __MODEL_HH__
