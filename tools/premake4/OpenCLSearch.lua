	if os.is("windows") or os.is("linux") then
		local amdopenclpath = os.getenv("AMDAPPSDKROOT")
		if( amdopenclpath ) then
			local clIncludePath = string.format("%s/include",amdopenclpath);
			local clIncludePath1
			if cl2path ~= nil then
				clIncludePath1 = amdopenclpath.."/include/"..cl2path
			end
			local clLibPath = string.format("%s/lib/x86",amdopenclpath);
			local clLibPath64 = string.format("%s/lib/x86_64",amdopenclpath);
			print("OpenCL_Path(AMD)", amdopenclpath)
			if (amdopenclpath) then
				includedirs {clIncludePath}
				if cl2path ~= nil then
					includedirs {clIncludePath1}
				end
				configuration "x64"
					libdirs {clLibPath64}
				configuration "x32"
					libdirs {clLibPath}
				configuration {}
				links {"OpenCL"}
			end
		else
			includedirs { "/opt/AMDAPP/include" }
			includedirs { "/opt/AMDAPP/include/opencl2.0" }
			configuration {"x64", "Debug"}
				libdirs {"/opt/AMDAPP/lib/x86_64"}
			configuration {"x32", "Debug"}
				libdirs {"/opt/AMDAPP/lib/x86"}
			configuration {"x64", "Release"}
				libdirs {"/opt/AMDAPP/lib/x86_64"}
			configuration {"x32", "Release"}
				libdirs {"/opt/AMDAPP/lib/x86"}
			configuration {}
			links {"OpenCL"}
		end
	end	

	if os.is("windows") or os.is("linux") then
		local nvidiaopenclpath = os.getenv("CUDA_PATH")
		if( nvidiaopenclpath ) then
			defines { "OPENCL_10" }
			nvidiaopenclpath = string.gsub(nvidiaopenclpath, "\\", "/");
			local clIncludePath = string.format("%s/include",nvidiaopenclpath);
			local clLibPath = string.format("%s/lib",nvidiaopenclpath);
			local clLibPath64 = string.format("%s/lib64",nvidiaopenclpath);
			print("OpenCL_Path(Nvidia)", nvidiaopenclpath)
			if (nvidiaopenclpath) then
				includedirs {clIncludePath}
		--		libdirs {clLibPath}
				configuration "x64"
					libdirs {clLibPath64}
				configuration "x32"
					libdirs {clLibPath}
				configuration {}
				links {"OpenCL"}
			end
		end
		
		local intelopenclpath = os.getenv("INTELOCLSDKROOT")
		if( intelopenclpath ) then
			defines { "INTEL_BUG_WORKROUND" }
			intelopenclpath = string.gsub(intelopenclpath, "\\", "/");
			local clIncludePath = string.format("%s/include",intelopenclpath);
			local clLibPath = string.format("%s/lib/x86",intelopenclpath);
			local clLibPath64 = string.format("%s/lib/x64",intelopenclpath);
			print("OpenCL_Path(Intel)", intelopenclpath)
			if (intelopenclpath) then
				includedirs {clIncludePath}
		--		libdirs {clLibPath}
				configuration "x64"
					libdirs {clLibPath64}
				configuration "x32"
					libdirs {clLibPath}
				configuration {}
				links {"OpenCL"}
			end
		end
	end
	
	if os.is("macosx") then
	   linkoptions{ "-framework OpenCL" }
	end




