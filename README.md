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