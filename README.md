WINGS: Writing with INtelligent Guidance and Suggestions
=====

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

