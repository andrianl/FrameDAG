#!/bin/bash

if [[ "$OSTYPE" == "darwin"* ]]; then
    # Для macOS можна генерувати як gmake, так і xcode4
    ./premake5 gmake2
else
    ./premake5 gmake2
fi

if [ $? -ne 0 ]; then
    echo "Premake5 execution failed. Make sure premake5 binary is in the root folder."
else
    echo "Makefiles generated. Type 'make' to build the project."
fi