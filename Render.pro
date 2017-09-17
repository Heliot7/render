# This project works with VS2013 and Qt 5.3.1/5.3.2 (32bits) - no initial manual settings needed
TEMPLATE			 = app
TARGET				 = Render
LANGUAGE			 = C++
CONFIG				+= qt thread warn_on console debug_and_release
QT					+= core gui opengl widgets

HEADERS				+=  lib/glew/include/GL/glew.h \
						lib/LodePNG/lodepng.h \
						include/ui/MainWindow.hpp \
						include/rendering/Render.hpp \
						include/rendering/Sampler.hpp \
						include/modelling/Model.hpp \ 
						include/modelling/Face.hpp \
						include/modelling/Vertex.hpp \
						include/modelling/Entity.hpp \
						include/modelling/BB.hpp \
						include/modelling/Texture.hpp \
						include/modelling/Tree.hpp
						
win32:HEADERS		+= lib/glew/include/GL/glew.h
				
SOURCES				+=	lib/glew/src/glew.c \
						lib/LodePNG/lodepng.cpp \
						src/main.cpp \
						src/ui/MainWindow.cpp \
						src/rendering/Render.cpp \
						src/rendering/Sampler.cpp \
						src/modelling/Model.cpp \
						src/modelling/Face.cpp \
						src/modelling/Vertex.cpp \
						src/modelling/Entity.cpp \
						src/modelling/BB.cpp \
						src/modelling/Texture.cpp

FORMS				+=	ui/MainWindow.ui

RESOURCES			+= 	data/resources.qrc

INCLUDEPATH			+=	include \
						lib \
						lib/glew/include \
						lib/soil/include \
						lib/assimp/include \
						lib/CImg \
						lib/LodePNG \
						lib/fbx/include
						
DEFINES				+=	GLEW_STATIC
						
win32 {
	!contains(QMAKE_TARGET.arch, x86_64) {
		message("Configuration 32 bits")
		LIBS +=			-Llib/SOIL/x86 -lSOIL
		CONFIG(debug, debug|release) {
			DESTDIR = bin\debug
			LIBS +=		-Llib/assimp/x86/debug -lassimp
			QMAKE_POST_LINK = copy lib\assimp\x86\debug\*.dll bin\debug
		}
		CONFIG(release, debug|release) {
			DESTDIR = bin\release
			LIBS +=		-Llib/assimp/x86/release -lassimp
			QMAKE_POST_LINK = copy lib\assimp\x86\release\*.dll bin\release
		}
	} else {
		message("Configuration 64 bits")
		LIBS +=			-Llib/SOIL/x64 -lSOIL
		CONFIG(debug, debug|release) {
			DESTDIR = bin\debug
			LIBS +=		-Llib/assimp/x64/debug -lassimp
			QMAKE_POST_LINK = copy lib\assimp\x64\debug\*.dll bin\debug
		}
		CONFIG(release, debug|release) {
			DESTDIR = bin\release
			LIBS +=		-Llib/assimp/x64/release -lassimp
			QMAKE_POST_LINK = copy lib\assimp\x64\release\*.dll bin\release
		}
	}
}

