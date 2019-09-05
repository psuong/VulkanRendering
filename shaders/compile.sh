#!/bin/bash

vulkan_sdk=$VULKAN_SDK

$vulkan_sdk/bin/glslangValidator -V shader.vert
$vulkan_sdk/bin/glslangValidator -V shader.frag
