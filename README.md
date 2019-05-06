# Vulkan Rendering #

Me playing around with the Vulkan API and providing some notes on basic Vulkan API.

## Table of Contents ##
* [Validation Layer](#Validation-Layers)
* [Window Surface](#Window-Surface)
* [Swap Chains](#Swap-Chains)
  * [Presentation Mode](#Presentation-Mode)
  * [Swap Extents](#Swap-Extents)
  * [Swap Chain Images](#Swap-Chain-Images)
* [Graphics Pipeline Basics](#Graphics-Pipeline-Basics)
  * [Shader Language](#Shader Language)

### Validation-Layers ###
Validation layers provide basic checking within Vulkan. Vulkan was designed to have minimal overhead so error checking is
done on the application layer instead of the Vulkan driver.

These can help with
* resource monitoring with memory allocation
* thread safety
* tracking Vulcan calls for profiling and playing

### Window Surface ###
Vulkan needs to be hooked up with the window library of your choice, whether it be QT or even GLFW. A `VK_KHR_surface`
exposes a `VkSurfaceKHR` object which is an abstract object that images are presented onto.

Window surface creation _actually_ affects which physical device is selected so it is deferred to a later step. Windows
are also optional and Vulkan allows off screen rendering without much of the workaround found in OpenGL.

### Swap Chains ###
Before an image gets rendered, the images are stored in render buffers. Or really in Vulkan, the swap chain infrastructure.
The images are stored in a series of queues before the image is presented to on screen.

**Swap Chain Optimality** can be determined via
* surface format (colour depth)
* presentation mode
* swap extent (resolution of images in the swap chain)

### Presentation Mode ###
Presentation mode can be considered one of the most important pieces of infrastructure within the swap chain pipeline. It
defines the conditions to show an image. There are _4_ possible presentation modes:

* `VK_PRESENT_MODE_IMMEDIATE_KHR` : Images submitted by the application are rendered immediately which may result in screen
tearing.
* `VK_PRESENT_MODE_FIFO_KHR`: Follows the classic FIFO infrastructure, an image at the front of the queue will be rendered,
and images are submitted to the back of the queue. The application is forced to wait _if_ the queue is full. Similar to V-Sync.
* `VK_PRESENT_MODE_FIFO_RELAXED_KHR`: Similar to the previous one except that if the queue was empty and the application
is stalled, then the image is rendered right away.
* `VK_PRESENT_MODE_MAILBOX_KHR`: Allows overwrite of the images that are currently within the queue. If the queue is full,
the previous images are replaced with the newly submitted ones. Can be used to for double buffering or triple buffering.

### Swap Extents ###
The swap extent is the resolution of the images and is usually equal to the resolution of the window that you're drawing. The
`VkSurfaceCapabilitiesKHR` is a struct which holds the range of possible resolutions.

### Swap Chain Images ##
* `VK_SHARING_MODE_EXCLUSIVE` - An image is owned by a particular queue. If another queue needs an image an explicit command
is needed to transfer the image from one queue to another.

* `VK_SHARING_MODE_CONCURRENT` - Images do not need to be owned by a particular queue to be used.

### Graphics Pipeline Basics ###
* Input Assembler - collects raw vertex data from the specified buffers and may use an index buffer to reduce copied vertices
* Vertex Shader - runs for every vertex and generally applies xfroms to turn vertex positions from model space to screen space.
* Tessellation Shader - allows you to divide geometry based on certain rules to increase mesh quality (e.g. brick walls
and staircases look less flat when nearby)
* Geometry shader - runs on every primitive and can discard it or output more primitives. Similar to tesselation shader,
 but geom shader performance is worse than tesselation shaders (unless on Intel Integrated Graphics? - I'll need to check this)
* Raserization - represents primitves into fragments. These are pixel elements that they fill on the frame buffer. Any
fragments outside of the screen are discard and the outputted by the vertex shader are interpolated across the fragments.
Fragments behind others are also discarded due to depth testing.
* Fragment Shader - invoked for every fragment that isn't discarded. Deterines which framebuffer the fragments are written
to and with which color and depth values.
* Color Blending - Applies operations to mix different fragments that map to the same pixel in the frame buffer.
 Fragments can simply overwrite each other, be additive or multiplicative.

Vertex Shader, Tessellation, Geometry Shader, and Fragment Shader are stages in the pipeline which can be programmed
and adjusted by the programmer.

Older APIs like OpenGL and Direct3D allow you to change any pipeline settings at will with calls like `glBlendFunc` and
`OMSetBlendState`. In *Vulkan*, the pipeline is almost completely immutable, so the pipeline must be created from scratch
if shader need to change. While less flexible on the surface, this allows for greater optimization and performance in the
long run.

### Shader Language ###
One of the interesting things about Vulkan is that the format Vulkan reads shader is via the SPIR-V format, which is a
bytecode format. The bytecode format allows for safe compilation amongst many platforms and is a good standard for Vulkan.
Luckily, for people rendering in Vulkan, we can still follow the GLSL format and write human readable shader code. There
is a compiler which compiles GLSL into bytecode and ensures that we write valid GLSL code.
