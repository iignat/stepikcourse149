FROM ubuntu:bionic

RUN apt-get update && apt-get install -y --no-install-recommends \
	build-essential \
	libevent-dev \
	libev-dev \
	libuv1-dev \
	libboost-all-dev \
	&& \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/bin/bash"]