FROM ubuntu:22.04

RUN DEBIAN_FRONTEND=noninteractive \
  && apt-get update \
  && apt-get upgrade \
  && apt-get -y --quiet --no-install-recommends install \
    apt-utils \
    autoconf \
    automake \
    bash-completion \
    build-essential \
    git \
    git-lfs \
    netcat \
    net-tools \
    rsync \
    software-properties-common \
    sudo \
    openssh-client \
    cmake-curses-gui \
    htop \
    gdb \
    sshpass \
    sshpass \
    python3 \
    python3-pip \
    tcpdump \
    libtool \
    libssl-dev \
    curl \
    libgtest-dev \
  && apt-get -y autoremove \
  && apt-get clean autoclean \
  && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

RUN pip3 install clang-format

ENV USERNAME=docker_user
ENV USER_UID=1000
ENV USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME && useradd -m --uid $USER_UID --gid $USER_GID $USERNAME \
    # add sudo support
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && usermod -a -G dialout $USERNAME

USER $USERNAME

ENV DISPLAY :1

CMD ["/bin/bash"]
