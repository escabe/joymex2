if isunix % Building on Linux
    mex('-v','src/joymex2.c','-lSDL');
else % Building on Windows
    mex('-v','src/joymex2.c','-ISDL/include',['-LSDL/bin/' computer('arch')],'-lSDL');
    copyfile(['SDL/bin/' computer('arch') '/SDL.dll'],'.');
end