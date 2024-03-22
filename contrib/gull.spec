#
# spec file for package gull
#
# Copyright (c) 2024 Hewlett Packard Enterprise Development, LP.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via https://bugs.opensuse.org/
#

%define git_sha 010cc97b0caaeed91f1a06ef414e13531f48e52a

Name:           gull
Version:        0.3_pre20230512
Release:        0
Summary:        Multi-node fabric-attached memory manager
License:        MIT
URL:            https://github.com/HewlettPackard/gull/
Source0:        https://github.com/HewlettPackard/gull/archive/%{git_sha}.tar.gz
BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  libboost_log-devel
BuildRequires:  libboost_thread-devel
BuildRequires:  libboost_system-devel
BuildRequires:  libboost_filesystem-devel
BuildRequires:  libboost_atomic-devel
BuildRequires:  libboost_regex-devel
BuildRequires:  libpmem-devel
BuildRequires:  yaml-cpp-devel

%description
Multi-node fabric-attached memory manager that provides simple abstractions for accessing and allocating NVM from fabric-attached memory 

%package        devel
Summary:        Development files for %{name}
Requires:       %{name} = %{version}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%prep
%setup -q -n %{name}-%{git_sha}

%build
%cmake
%cmake_build

%install
%cmake_install
# find %{buildroot} -type f -name "*.la" -delete -print

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license COPYING
%doc README.md
%{_libdir}/*.so

%files devel
%{_includedir}/*
%{_libdir}/cmake/nvmm/*

%changelog
