include(FetchContent)

set(WEBGPU_BACKEND "WGPU" CACHE STRING "Backend implementation of WebGPU.")

if (NOT TARGET webgpu)
	string(TOUPPER ${WEBGPU_BACKEND} WEBGPU_BACKEND_U)

	if (EMSCRIPTEN OR WEBGPU_BACKEND_U STREQUAL "EMSCRIPTEN")

		FetchContent_Declare(
			webgpu-backend-emscripten
			GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
			GIT_TAG        emscripten-v3.1.45
			GIT_SHALLOW    TRUE
		)
		FetchContent_MakeAvailable(webgpu-backend-emscripten)

	else()

		FetchContent_Declare(
			webgpu-backend-dawn
			GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
			GIT_TAG        dawn-5869
			GIT_SHALLOW    TRUE
		)
		FetchContent_MakeAvailable(webgpu-backend-dawn)

	endif()
endif()