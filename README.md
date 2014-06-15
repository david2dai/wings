WINGS
=====
# Introduction
Welcome to use WINGS (Writing with INtelligent Guidance and Suggestions). WINGS is a Chinese input method extended on IBus-Pinyin with intelligent writing assistance. It focuses on providing real-time and intelligent writing suggestions, including syntacticly and semanticly related words and contextually related sentences, to writers during their writing.


# Build and Install
## 1. Clucene
> mkdir build          (Under the root folder clucene_final)

> cd build 

> cmake -DENABLE_ASCII_MODE=ON -DCMAKE_INSTALL_PREFIX=/usr ..

> make

> sudo make install

## 2. recommendlib
> make

> sudo make install

## 3. ibus
### 3.1. Copy the files in the following subfolders to ibus-1.4.2 by modifying the scrpit file "updatecode" 
> daemon

> ibuslib

> ibuspython

> ui

### 3.2. Dependent libraries
> apt-get install libgtk+2.0-dev libgtk-3-dev libgirepository1.0-dev libdbus-glib-1-dev libdconf-dev libgconf2-dev libdconf-dbus-1-dev dconf-tools libvala-0.16-dev libsqlite3-dev libnotify-dev python-gobject-dev python-notify python-dev python-dbus-dev python-enchant python-xdg uuid-dev iso-codes sqlite3 valac valabind gnome-common gtk-doc-tools

### 3.3. Build and Install
> ./autogen.sh --prefix=/usr --sysconfdir=/etc --libdir=/usr/lib

> make 

> sudo make install

## 4. ibus-pinyin
### 4.1. Copy the files in the following subfolders to ibus-pinyin-1.4.0 by modifying the scrpit file "updatecode" 
> engine

### 4.2. Build and Install
> ./autogen.sh --prefix=/usr

> make

> sudo make install

## 5. index
index subfolder is used for creating sentences index and testing search.

## 6. Words and Sentences resources
Put the words and sentences resources in the following url to "/usr/share/ibus-pinyin/resources/" of your Ubuntu System.
> http://pan.baidu.com/s/1bns3tUZ


# Use WINGS
## 1. Remove the old ibus library (.so files) from the system
> sudo rm /usr/lib/x86_64-linux-gnu/libibus-1.0.so.0.401.0

> sudo rm /usr/lib/x86_64-linux-gnu/libibus-1.0.so.0

## 2. Add Chinese Pinyin to input method
Enter the following command in terminal: 
> ibus-setup

There will be a window called "IBus Preferences" appearing, select "Input Method" page. 
Click "Select an input method" list and add "Chinese-Pinyin" to system input method list.

## 3. Select "IBus" as system's default input method
Enter the following command in terminal: 
> im-switch

Then there will be a window called "Input Method Switcher" coming out.
Select "Use IBus(ibus)" and click OK button.
Logout system and re-Login

## 4. Use "Ctrl+Space" to switch the input method. 
In Terminal/Gedit/Browser editing area, use Ctrl+Space to enable WINGS and start your input. 
