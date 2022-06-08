%if 0%{!?rpm_device:1}
%define rpm_device %{device}
%endif

%if 0%{!?kernel_arch:1}
%define kernel_arch arm
%endif

# The prjconf should have an ExportFilter like this (mer/sailfish has this):
#   ExportFilter: \.armv7hl\.rpm$ armv8el
# We lie about our architecture and allows OBS to cross-publish this 486 cross-built spec to the armv7hl repos
%if 0%{?device_target_cpu:1}
%define _target_cpu %{device_target_cpu}
%else
%define _target_cpu %{_arch}
%endif

%if 0%{!?defconfig:1}
%define defconfig defconfig
%endif

%if 0%{!?source_directory:1}
%define source_directory linux/
%endif

%if 0%{!?build_Image:1}
%define build_Image 1
%endif

%if 0%{!?build_uImage:1}
%define build_uImage 0
%endif

%if 0%{!?build_zImage:1}
%define build_zImage 0
%endif

%if 0%{!?build_modules:1}
%define build_modules 1
%endif

%define _tmpdir tmp

Name:           kernel-adaptation-%{rpm_device}
Summary:        Kernel adaptation for %{rpm_device}
Group:          System
License:        GPLv2
Version:        2.6
Release:        1
Provides:       kernel
Source:         %{name}-%{version}.tar.bz2
Requires(pre):  kmod >= 9
BuildRequires:  pkgconfig(ncurses)
BuildRequires:  pkgconfig(openssl)
BuildRequires:  mer-kernel-checks
#BuildRequires:  kmod >= 9
#BuildRequires:  perl
#BuildRequires:  fdupes
BuildRequires:  flex
BuildRequires:  bison
BuildRequires:  bc
%if 0%{?crossbuild:1}
BuildRequires:  cross-%{crossbuild}-gcc
BuildRequires:  cross-%{crossbuild}-binutils
BuildRequires:  cross-%{crossbuild}-as
%endif

%description
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build

rm -rf %{_tmpdir}
mkdir -p %{_tmpdir}

export ARCH=%{kernel_arch}
export LOCALVERSION="-%{device}"

%if 0%{?crossbuild:1}
export CROSS_COMPILE=%{crossbuild}-meego-linux-gnu-
# FIXME? Shouldn't be needed
export PATH=/opt/cross/bin/:$PATH
%endif

defconfig_name=$(basename %{defconfig})
[ -e "./%{defconfig}" ] && cp -f ./%{defconfig} %{source_directory}/arch/%{kernel_arch}/configs/$defconfig_name

%if 0%{?apply_patches:1}
#Apply any patches
sh patches/apply-patches.sh %{source_directory}
%endif

make -C %{source_directory} clean
make -C %{source_directory} defconfig KBUILD_DEFCONFIG=$defconfig_name

%if %{build_Image}
make -C %{source_directory} -j$(nproc) Image
%endif

%if %{build_uImage}
make -C %{source_directory} -j$(nproc) uImage
%endif

%if %{build_zImage}
make -C %{source_directory} -j$(nproc) zImage
%endif

%if %{build_modules}
make -C %{source_directory} -j$(nproc) modules
%endif

%if 0%{?devicetrees:1}
make -C %{source_directory} -j$(nproc) dtbs
%endif

%install

mkdir -p %{buildroot}/boot

%if %{build_Image}
cp %{source_directory}/arch/%{kernel_arch}/boot/Image %{buildroot}/boot/Image
%endif

%if %{build_uImage}
cp %{source_directory}/arch/%{kernel_arch}/boot/uImage %{buildroot}/boot/uImage
%endif

%if %{build_zImage}
cp %{source_directory}/arch/%{kernel_arch}/boot/zImage %{buildroot}/boot/zImage
%endif

%if 0%{?devicetrees:1}
for dtb in %{devicetrees}; do
	mkdir -p %{buildroot}/boot/$(basename $(dirname $dtb))
	cp %{source_directory}/arch/%{kernel_arch}/boot/dts/$dtb %{buildroot}/boot/$dtb
	cp %{buildroot}/boot/*/*.dtb %{buildroot}/boot/
done
%endif

mkdir -p %{buildroot}/lib/modules
make -C %{source_directory} modules_install INSTALL_MOD_PATH=%{buildroot}

# Build filelist
find %{buildroot} ! \( -type d \) | sed -e s,"%{buildroot}",,g> %{_tmpdir}/kernel-adaptation.files

%files -f %{_tmpdir}/kernel-adaptation.files
