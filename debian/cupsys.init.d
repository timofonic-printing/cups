#! /bin/sh
#
#		Written by Miquel van Smoorenburg <miquels@cistron.nl>.
#		Modified for Debian GNU/Linux
#		by Ian Murdock <imurdock@gnu.ai.mit.edu>.
#
# Version:	@(#)skeleton  1.8  03-Mar-1998  miquels@cistron.nl
#
# This file was automatically customized by dh-make on Sun,  3 Oct 1999 20:58:02 -0500

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/cupsd
NAME=cupsd
PIDFILE=/var/run/cups/$NAME.pid
DESC="Common Unix Printing System"

test -f $DAEMON || exit 0

set -e

# Get the timezone set.
if [ -e /etc/timezone ]; then
    TZ=`cat /etc/timezone`
    export TZ
fi

case "$1" in
  start)
	echo -n "Starting $DESC: $NAME"
	chown root:lpadmin /usr/share/cups/model 2>/dev/null || true
	chmod 3775 /usr/share/cups/model 2>/dev/null || true
	start-stop-daemon --start --quiet --pidfile "$PIDFILE" --exec $DAEMON
	echo "."
	;;
  stop)
	echo -n "Stopping $DESC: $NAME"
	start-stop-daemon --stop --quiet --user root --retry TERM/10 --oknodo --pidfile $PIDFILE --name $NAME
	echo "."
	;;
  reload)
	echo -n "Reloading $DESC: $NAME"
	start-stop-daemon --stop --quiet --pidfile $PIDFILE --name $NAME --signal 1
	echo "."
	;;
  restart|force-reload)
	echo -n "Restarting $DESC: $NAME"
	if start-stop-daemon --stop --quiet --user root --retry TERM/10 --oknodo --pidfile $PIDFILE --name $NAME; then
		start-stop-daemon --start --quiet --pidfile "$PIDFILE" --exec $DAEMON
	fi
	echo "."
	;;
  status)
	echo -n "Status of $DESC: "
	if [ ! -r "$PIDFILE" ]; then
		echo "$NAME is not running."
		exit 3
	fi
	if read pid < "$PIDFILE" && ps -p "$pid" > /dev/null 2>&1; then
		echo "$NAME is running."
		exit 0
	else
		echo "$NAME is not running but $PIDFILE exists."
		exit 1
	fi
	;;
  *)
	N=/etc/init.d/${0##*/}
	echo "Usage: $N {start|stop|restart|force-reload|status}" >&2
	exit 1
	;;
esac

exit 0
