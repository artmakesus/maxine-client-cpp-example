#include <cstdio>

#include <QtCore/QCoreApplication>
#include <QtDBus/QtDBus>
#include <QSharedMemory>
#include <QtMath>

static const QString DBUS_SERVICE_NAME = "com.artmakesus.maxine";
static const int WIDTH = 1024;
static const int HEIGHT = 768;

void draw(quint32 *data)
{
	static int xx = 1;

	for (quint32 y = 0; y < HEIGHT; y++) {
		for (quint32 x = 0; x < WIDTH; x++) {
			if (((x + xx) / 64) % 2 == 0) {
				data[x + y * WIDTH] = 0xFFFFFFFF;
			} else {
				data[x + y * WIDTH] = 0;
			}
		}
	}

	xx = (xx + 8) % 128;
}

void manipulate(QDBusInterface &iface, const QString &key)
{
	for (;;) {
		// FIXME: QSharedMemory shouldn't be recreated again and again but seems like it's the only way for the program to work
		auto shm = new QSharedMemory(key);
		shm->attach();
		shm->lock();
		auto data = static_cast<quint32*>(shm->data());
		draw(data);
		shm->unlock();
		shm->detach();
		delete shm;

		iface.call("invalidateSharedTexture", 0);
		QThread::msleep(16);
	}
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	if (!QDBusConnection::sessionBus().isConnected()) {
		fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
				"To start it, run:\n"
				"\teval `dbus-launch --auto-syntax`\n");
		return 1;
	}

	QDBusInterface iface(DBUS_SERVICE_NAME, "/", "", QDBusConnection::sessionBus());
	if (iface.isValid()) {
		QDBusReply<bool> reply = iface.call("createSharedTexture", "mytexture", 0, WIDTH, HEIGHT);
		if (reply.isValid()) {
			printf("Reply was: %s\n", qPrintable(reply.value()));
			manipulate(iface, "mytexture");
			return 0;
		}

		fprintf(stderr, "Call failed: %s\n", qPrintable(reply.error().message()));
		return 1;
	}

	fprintf(stderr, "%s\n", qPrintable(QDBusConnection::sessionBus().lastError().message()));
	return 1;
}
