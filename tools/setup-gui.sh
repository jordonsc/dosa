#!/usr/bin/env bash

app=$(python3 -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")

echo ####################
echo   INSTALL PRE-REQS
echo ####################
sudo apt update
sudo apt install pkg-config libgl1-mesa-dev libgles2-mesa-dev \
    libgstreamer1.0-dev \
    gstreamer1.0-plugins-{bad,base,good,ugly} \
    gstreamer1.0-{omx,alsa} libmtdev-dev \
    xclip xsel libjpeg-dev

echo ####################
echo    INSTALL SDL2
echo ####################
wget https://libsdl.org/release/SDL2-2.0.10.tar.gz
tar -zxvf SDL2-2.0.10.tar.gz
pushd SDL2-2.0.10
./configure --enable-video-kmsdrm --disable-video-opengl --disable-video-x11 --disable-video-rpi
make -j$(nproc)
sudo make install
popd

echo ####################
echo  INSTALL SDL2-IMAGE
echo ####################
wget https://libsdl.org/projects/SDL_image/release/SDL2_image-2.0.5.tar.gz
tar -zxvf SDL2_image-2.0.5.tar.gz
pushd SDL2_image-2.0.5
./configure
make -j$(nproc)
sudo make install
popd

echo ####################
echo  INSTALL SDL2-MIXER
echo ####################
wget https://libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.4.tar.gz
tar -zxvf SDL2_mixer-2.0.4.tar.gz
pushd SDL2_mixer-2.0.4
./configure
make -j$(nproc)
sudo make install
popd

echo ####################
echo   INSTALL SDL2-TTF
echo ####################
wget https://libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz
tar -zxvf SDL2_ttf-2.0.15.tar.gz
pushd SDL2_ttf-2.0.15
./configure
make -j$(nproc)
sudo make install
popd


echo ####################
echo     CONFIGURING...
echo ####################
sudo ldconfig -v
sudo adduser "$USER" render

echo ####################
echo    INSTALL KIVY
echo ####################
pip3 install "kivy[base]"
sudo pip3 install "kivy[base]"
