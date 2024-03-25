local project = {
	lang = {
		langtype = "c++",
		std = "gnu++20",
	},
	compiler = nil, -- default compiler for host operating system
}

local srcs = {
	"./src/main.c",
	"./src/foo.c",
}

--aedif.os.mkdir("obj")

for _, src in ipairs(srcs) do
	aedif.compile_obj(project, src)
end
