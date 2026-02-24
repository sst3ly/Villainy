# Villainy

Villainy is a C++ graphics library and application framework built on Vulkan and GLFW, designed for creating high-performance, cross-platform graphical applications.

## Features
- Vulkan-based rendering
- GLFW window and input management
- Modular architecture (Context, Window, Swapchain, Renderer, etc.)
- Support for custom pipelines and shaders
- Example code for quick start

## Requirements
- C++17 or later
- Vulkan SDK (with MoltenVK for macOS)
- GLFW
- CMake 3.10+

## Building

1. Install dependencies - Vulkan SDK
2. Clone the repository:
   ```sh
   git clone <your-repo-url>
   cd Villainy
   ```
3. Build with CMake:
   ```
   ./build.sh
   ```
   generates build/libVillainyLib.a and build/villainy demo executable

   On Linux, install Vulkan SDK and GLFW 3
   On Mac, install Vulkan SDK
   Windows support coming soon
5. Run the example/your code after modifying it:
   ```
   ./run.sh
   ```

## Project Structure
```
Villainy/
├── src/
│   ├── main.cpp           # Main application entry
│   ├── example.cpp        # Example usage of the library
│   └── villainy/          # Core library source
│       ├── context.*      # Vulkan context management
│       ├── window.*       # Window and surface management
│       ├── render.*       # Rendering pipeline
│       ├── ...            # Other modules
├── external/              # External dependencies (GLFW, glm, stb, etc.)
├── CMakeLists.txt         # Build configuration
├── run.sh                 # Run helper script
└── README.md              # Project documentation
```

## Example Usage
See `src/main.cpp` for a minimal example.

When making shaders, use glslc or an alternative to compile .spv files from the shader source files.


## License
MIT License

---

For more information, see the source code and comments. Contributions and issues are welcome!
