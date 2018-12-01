# SHIFT WebClient Guide


[header]: # "To generate a html version of this document:"
[pandoc]: # "pandoc README.md -c ../Templates/github.css -o README.html -s --self-contained"

## Required Libraries

**Please refer to the other required libraries in the SHIFT Developer Guide.**

### SHIFT Miscellaneous Utils & Core Client

Use the installer in the root folder of the SHIFT project:

- In the Terminal: `sudo ./install -m LM LC`

### Apache Thrift:

- In the Terminal: `sudo apt-get install bison flex libevent-dev`

Choose a location to keep the Apache Thrift source files (for debugging purposes), e.g. a "C++" folder in your home directory, and then:

``` bash
curl -O archive.apache.org/dist/thrift/0.11.0/thrift-0.11.0.tar.gz
tar -zxf thrift-0.11.0.tar.gz
cd thrift-0.11.0
./configure --prefix=/usr/local/
make
sudo make install
cd ..
rm thrift-0.11.0.tar.gz
```

### ZeroMQ:

Choose a location to keep the ZeroMQ source files (for debugging purposes), e.g. a "C++" folder in your home directory, and then:

``` bash
git clone https://github.com/zeromq/libzmq.git
cd libzmq
mkdir build
cd build
cmake ..
sudo make install
```

In the same location as before (`cd ../..`), do the following to install the C++ binding for ZeroMQ: 

``` bash
git clone https://github.com/zeromq/cppzmq.git
cd cppzmq
mkdir build
cd build
cmake ..
sudo make install
```

---

## PHP Installation

``` bash
sudo add-apt-repository ppa:ondrej/php
sudo apt-get update
sudo apt-get install php7.0-cli php7.0-fpm php7.0-pgsql php7.0-xml php-zmq
```

---

## Nginx Installation

- In the Terminal: `sudo apt-get install nginx`

Add the following information to `/etc/nginx/common.conf`, substituting the path to the cloned repository in `SITE_ROOT`:

``` bash
index index.php index.html index.htm;
gzip on;
gzip_http_version 1.0;
gzip_comp_level 9;
gzip_disable "MSIE [1-6].";
gzip_types text/css text/javascript image/jpeg image/gif image/png application/x-javascript;
proxy_hide_header Vary;
location ~* \.(jpg|jpeg|gif|png|swf|rar|zip|css|ico|js|ico|cert|xml|txt)$ {
    access_log off;
    expires 30d;
    break;
}
location ~ \.php {
    fastcgi_pass unix:/var/run/php/php7.0-fpm.sock;
    fastcgi_index index.php;
    include fastcgi_params;
    fastcgi_param DB_DEFAULT_USER root;
    fastcgi_param DB_DEFAULT_HOST 127.0.0.1;
    fastcgi_param DB_DEFAULT_PASS 123456;
    fastcgi_param SITE_ROOT <path_to_cloned_repository>/WebClient/frontend;
    fastcgi_param env production;
}
```

Remove the default Nginx site information and create one for SHIFT:

- In the Terminal: 

    `sudo rm /etc/nginx/sites-enabled/default`

    `sudo vim /etc/nginx/sites-available/shift`

with the following content, substituting the path to the cloned repository in `root`:

``` bash
server {
    server_name www.myshift.com myshift.com;
    root <path_to_cloned_repository>/WebClient/frontend/public;
    index index.php index.html index.htm;
    
    error_log /tmp/error.log;
    access_log /var/log/nginx/myshift_com_access.log;
    include /etc/nginx/common.conf;
}
```

Create a symlink under Nginx's `sites-enabled` folder to activate SHIFT's site with the site information you just created:

- In the Terminal: 

    `cd /etc/nginx/sites-enabled`

    `sudo ln -s ../sites-available/shift shift`

    `sudo service nginx restart`

At this point, you are supposed to see something in your browser when pointing to localhost (or 127.0.0.1). There is a known issue in Ubuntu 18.04, however, and there is a possibility nothing will show up. In this case, paste the following in the bottom of `/etc/nginx/fastcgi_params`:

``` bash
fastcgi_param  PATH_TRANSLATED    $document_root$fastcgi_script_name;
```

After saving the file, restart Nginx:

- In the Terminal: `sudo service nginx restart`

For more information on this known issue, refer to this [question](https://stackoverflow.com/questions/15423500/nginx-showing-blank-php-pages) in Stack Overflow.

---

## Composer Installation

Follow the instructions in <https://getcomposer.org/download/>, and move the composer installer to the `/usr/local/bin` folder:

- In the Terminal: `sudo mv composer.phar /usr/local/bin/composer`

---

## SHIFT WebClient Installation

**Before installing the SHIFT WebClient, you should create proper configuration files.**

The default configuration files (pointing to localhost) can be copiedby using the `copy_config.sh` script contained in the `Scripts` folder or by using the `-f` parameter in the installer. 

To install the WebClient:

- In the Terminal: `sudo ./install.sh`

To uninstall the WebClient:

- In the Terminal: `sudo ./install.sh -u`

For additional options:

- In the Terminal: `./install.sh -h`

---

## Fixing the Shared Library Cache

When running the WebClient for the first time, it is possible the system will not find our shared libraries. To fix such problem:

- In the Terminal: `sudo ldconfig`

---
