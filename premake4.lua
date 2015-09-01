function fileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

solution "OCLRadixSort"
	configurations { "Debug", "Release" }
	language "C++"
	flags { "NoMinimalRebuild", "EnableSSE", "EnableSSE2" }
	-- define common includes
	includedirs { ".","./contrib/include" }

	-- perform OS specific initializations
	local targetName;
	if os.is("macosx") then
        targetName = "osx"
		buildoptions{"-std=c++11 -Wno-c++11-narrowing"}
	end
	platforms {"x64"}

	if os.is("linux") then
        targetName = "linux"
		defines{ "__LINUX__" }
		-- Add -D_GLIBCXX_DEBUG for C++ STL debugging
		buildoptions{"-fopenmp -fPIC -Wno-deprecated-declarations"}
		buildoptions{"-std=c++11"}
		links{ "gomp" }
	end

	if os.is("windows") then
		targetName = "win"
		defines{ "WIN32" }
		buildoptions { "/MP"  }
		buildoptions { "/wd4305 /wd4244 /wd4661"  }
		defines {"_CRT_SECURE_NO_WARNINGS"}
		configuration {"Release"}
		buildoptions { "/openmp"  }
		configuration {}
	end

	configuration "Debug"
		defines { "_DEBUG" }
		flags { "Symbols" }
	includedirs { "./dist/debug/include" }
	configuration "Release"
		defines { "NDEBUG" }
	includedirs { "./dist/release/include" }

	configuration {"x64", "Debug"}
		targetsuffix "64D"
	libdirs { "./dist/debug/lib/x86_64" }
	targetdir "./dist/debug/bin/x86_64"
	configuration {"x64", "Release"}
		targetsuffix "64"
	libdirs { "./dist/release/lib/x86_64" }
	targetdir "./dist/release/bin/x86_64"
		flags { "Optimize" }

	configuration "x64"
		libdirs { "./contrib/lib/"..targetName.."64" }
		defines {"_X64"}
	configuration {} -- back to all configurations

	-- find and add path to Opencl headers
	cl2path = "opencl1.2"
	if _OPTIONS["cl2"] then
		cl2path = "opencl2.0"
		defines {"TH_CL2"}
	end
	dofile ("./tools/premake4/OpenCLSearch.lua" )

	-- generate the projects
	project "UnitTest"
		kind "ConsoleApp"
		location "UnitTest"
	    configuration {}
		
		files { "Adl/**.cpp", "Adl/**.h", "Adl/**.inl" }
		files { "UnitTest/main.cpp", "contrib/src/gtest-1.6.0/**" } 
		files { "Tahoe/**.cpp", "Tahoe/**.h", "Tahoe/**.cl" }
		defines { "TH_LOG_LEVEL=3", "TH_UNIT_TEST", "TH_NON_WINDOW_TEST" }
	        
		if os.is("linux") then
--			links {"GL", "X11", "Xi", "Xxf86vm", "Xrandr", "pthread" }
		end
			
		if _ACTION == "vs2012" then
			defines{ "GTEST_HAS_TR1_TUPLE=0" }
		end
		
		if os.is("macosx") then
			linkoptions{ "-framework Cocoa", "-framework OpenGL", "-framework IOKit" }
			-- for gtest
			buildoptions{"-DGTEST_USE_OWN_TR1_TUPLE=1"}
		end

		configuration {}
