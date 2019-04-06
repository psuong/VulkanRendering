# Vulkan Rendering #

Me playing around with the Vulkan API and providing some notes on basic Vulkan API.

## Table of Contents ##
* [Validation Layer](#Validation-Layers)

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
