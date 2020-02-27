#####################################################################################################
# Build the QuickFIX library.
#####################################################################################################
FROM ubuntu:18.04 as quickfix

RUN apt-get update

RUN apt-get install -y \
        cmake \
        git \ 
        libboost-all-dev \
        libcpprest-dev

RUN git clone https://github.com/hanlonlab/quickfix.git
WORKDIR quickfix
RUN mkdir build
WORKDIR build
RUN cmake \
         -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -Wno-deprecated" \
         -DCMAKE_BUILD_TYPE=RelWithDebInfo \
         -DHAVE_SSL=ON \
         ..
RUN make && make install

#####################################################################################################
# Copy the shared object libraries from QuickFix and build the service in shift-main.
#####################################################################################################
FROM ubuntu:18.04 as shift-main

RUN mkdir /usr/local/include/quickfix
RUN mkdir /usr/local/share/quickfix
COPY --from=quickfix /usr/local/include/quickfix /usr/local/include/quickfix
COPY --from=quickfix /usr/local/share/quickfix /usr/local/share/quickfix
COPY --from=quickfix /usr/local/lib/libquickfix.so.16.0.1 /usr/local/lib/
RUN ln -s /usr/local/lib/libquickfix.so.16.0.1 /usr/local/lib/libquickfix.so.16
RUN ln -s /usr/local/lib/libquickfix.so.16.0.1 /usr/local/lib/libquickfix.so

RUN apt-get update && \
    apt-get install -y \
        autoconf \
        build-essential \
        libboost-all-dev \
        libcpprest-dev \
        libcurl4-openssl-dev \
        libssl-dev \
        postgresql-server-dev-all \
        uuid-dev \
        cmake \
        doxygen \
        pandoc \
        pkg-config && \
    apt-get clean

RUN mkdir -p /shift/shift-main
COPY . /shift/shift-main/

WORKDIR /shift/shift-main
RUN find . -type f -name CMakeCache.txt | xargs rm -f

RUN ./install.sh -f

RUN ldconfig

# TODO - replace me with a "real" run command
CMD ["bash"]
