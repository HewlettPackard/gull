FROM opensuse/leap:15.5

RUN --mount=type=cache,target=/var/cache/zypp \
  set -ex ; \
  sed -i -e 's|http:|https:|g' /etc/zypp/repos.d/* ; \
  zypper --non-interactive install \
    cmake \
    gcc-c++ \
    libboost_log-devel \
    libboost_thread-devel \
    libboost_system-devel \
    libboost_filesystem-devel \
    libboost_atomic-devel \
    libboost_regex-devel \
    libpmem-devel \
    yaml-cpp-devel \
    rpmbuild
