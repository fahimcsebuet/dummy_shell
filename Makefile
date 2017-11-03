SOURCE=*.cpp
OBJECTS=
OUTPUT=output/myshell

MYLS_SOURCE=Myls/*.cpp
MYLS_OUTPUT=Myls/output/myls
#INCLUDES=

#LIBRARIES=

CXX=g++
CXXFLAGS=-Wall -ansi -pedantic

all: $(OUTPUT) $(MYLS_OUTPUT)

$(OUTPUT): $(SOURCE)
	#$(CXX) -I$(INCLUDES) $(SOURCE) -o$(OUTPUT) -l$(LIBRARIES)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o$(OUTPUT)

$(MYLS_OUTPUT): $(MYLS_SOURCE)
	$(CXX) $(CXXFLAGS) $(MYLS_SOURCE) -o$(MYLS_OUTPUT)

clean:
	rm -f $(OUTPUT)
	rm -f $(MYLS_OUTPUT)
