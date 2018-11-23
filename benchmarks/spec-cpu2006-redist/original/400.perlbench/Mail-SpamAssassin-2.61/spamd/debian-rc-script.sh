#! /bin/sh

# Spamd init script
# June 2002
# Duncan Findlay

# Based on skeleton by Miquel van Smoorenburg and Ian Murdock

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
DAEMON=/usr/sbin/spamd
NAME=spamd
SNAME=spamassassin
DESC="SpamAssassin Mail Filter Daemon"
PIDFILE="/var/run/$NAME.pid"
PNAME="spamd"
DOPTIONS="-d --pidfile=$PIDFILE"

# Defaults - don't touch, edit /etc/default/spamassassin
ENABLED=0
OPTIONS=""

test -f /etc/default/spamassassin && . /etc/default/spamassassin

test "$ENABLED" != "0" || exit 0

test -f $DAEMON || exit 0

set -e

case "$1" in
  start)
	echo -n "Starting $DESC: "
	start-stop-daemon --start --pidfile $PIDFILE --name $PNAME \
	    --startas $DAEMON -- $OPTIONS $DOPTIONS

	echo "$NAME."
	;;
  stop)
	echo -n "Stopping $DESC: "

	start-stop-daemon --stop --pidfile $PIDFILE --name $PNAME

	echo "$NAME."
	;;
  restart|force-reload)
	echo -n "Restarting $DESC: "
	start-stop-daemon --stop --pidfile $PIDFILE --name $PNAME --retry 5
	start-stop-daemon --start --pidfile $PIDFILE --name $PNAME \
	    --startas $DAEMON -- $OPTIONS $DOPTIONS

	echo "$NAME."
	;;
  *)
	N=/etc/init.d/$SNAME
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
