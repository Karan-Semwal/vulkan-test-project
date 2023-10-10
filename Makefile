default:
	g++ -std=c++20 -o test.exe test.cpp vulkan_backend.cpp imgui\imgui.cpp imgui\imgui_impl_glfw.cpp imgui\imgui_impl_vulkan.cpp imgui\imgui_demo.cpp  imgui\imgui_tables.cpp imgui\imgui_widgets.cpp imgui\imgui_draw.cpp glfw3.dll vulkan-1.dll -O3
debug:
	g++ -std=c++20 -o test.exe test.cpp vulkan_backend.cpp imgui\imgui.cpp imgui\imgui_impl_glfw.cpp imgui\imgui_impl_vulkan.cpp imgui\imgui_demo.cpp  imgui\imgui_tables.cpp imgui\imgui_widgets.cpp imgui\imgui_draw.cpp glfw3.dll vulkan-1.dll -Wall -Wextra -Werror -pedantic -Og -g
