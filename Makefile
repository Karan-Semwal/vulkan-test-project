test_debug:
	g++ -std=c++20 -o test.exe test.cpp vulkan_backend.cpp imgui/imgui.cpp imgui/imgui_impl_glfw.cpp imgui/imgui_impl_vulkan.cpp imgui/imgui_demo.cpp  imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui_draw.cpp D:\Python\vulk\glfw3.dll C:\Windows\System32\vulkan-1.dll -Wall -Wextra -Werror -pedantic -Og -g
test:
	g++ -std=c++20 -o test.exe test.cpp vulkan_backend.cpp imgui/imgui.cpp imgui/imgui_impl_glfw.cpp imgui/imgui_impl_vulkan.cpp imgui/imgui_demo.cpp  imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui_draw.cpp D:\Python\vulk\glfw3.dll C:\Windows\System32\vulkan-1.dll -O3
