#include <cstdio>

#include <QtCore/QCoreApplication>
#include <QtDBus/QtDBus>
#include <QSharedMemory>
#include <QtMath>

const QString DBUS_SERVICE_NAME = "com.artmakesus.maxine";

void manipulate(QDBusInterface &iface, const QString &key)
{
	quint32 t = 0;
	for (;;) {
		auto shm = new QSharedMemory(key);
		shm->attach();
		shm->lock();
		auto data = static_cast<quint32*>(shm->data());
		for (auto y = 0; y < 768; y++) {
			for (auto x = 0; x < 1024; x++) {
				data[x + y * 1024] = qSin(x * t * 20) * qCos(y * t * 10) * 0xFFFFFFFF;
			}
		}
		shm->unlock();

		iface.call("invalidateSharedTexture", 0);
		
		QThread::msleep(16);
		t += 16;
		delete shm;
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
		QDBusReply<bool> reply = iface.call("createSharedTexture", "mytexture", 0, 1024, 768);
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
