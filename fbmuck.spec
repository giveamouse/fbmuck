Summary: 	The FuzzBall TinyMUCK online chat/MUD server
Name: 		fbmuck
Version: 	6.00b4
Release: 	1
Group: 		Amusements/Games
Copyright: 	GPL
Url: 		http://sourceforge.net/projects/fbmuck/
Source:		http://prdownloads.sourceforge.net/fbmuck/%{name}-%{version}.tar.gz
Packager: 	Revar Desmera <revar@belfry.com>
Vendor:		FuzzBall Software
BuildRoot: 	/var/tmp/%{name}_root


%description 
FuzzBall Muck is a networked multi-user MUD chat server. It is
user-extensible, and supports advanced features such as GUI dialogs,
through close client-server cooperation with Trebuchet or other
clients that support MCP-GUI.

This is the FuzzBall Muck server daemon program.

If you are running this server with a new database.  You should
connect to the server with a MUD client program (or even just telnet),
then log in with the command 'connect #1 potrzebie'.  You should
immediately change your password with the command
'@password potrzebie=YOURNEWPASSWORD'.

%prep
%setup

%build
./configure --prefix=/usr
make CFLAGS="$RPM_OPT_FLAGS"

%install
make prefix=$RPM_BUILD_ROOT/usr install

%files
%defattr(-,root,root)
%doc docs/COPYING INSTALL README
/etc/fbmucks
/etc/rc.d/init.d/fbmuck
/etc/rc.d/rc0.d/K20fbmuck
/etc/rc.d/rc1.d/K20fbmuck
/etc/rc.d/rc2.d/K20fbmuck
/etc/rc.d/rc3.d/S82fbmuck
/etc/rc.d/rc4.d/S82fbmuck
/etc/rc.d/rc5.d/S82fbmuck
/etc/rc.d/rc6.d/K20fbmuck
/usr/bin/fbmuck
/usr/bin/fb-resolver
/usr/bin/fb-topwords
/usr/bin/fb-olddecompress
/usr/bin/fbhelp
/usr/bin/fb-announce
#/usr/sbin/fb-addmuck
/usr/share/fbmuck/help.txt
/usr/share/fbmuck/man.txt
/usr/share/fbmuck/mpihelp.txt
/usr/share/fbmuck/restart-script
#/usr/share/fbmuck/starter_dbs/basedb.tar.gz
#/usr/share/fbmuck/starter_dbs/minimaldb.tar.gz


%changelog

* Wed Apr 18 2001 Revar Desmera <revar@belfry.com>
- First try at making the packages

