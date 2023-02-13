#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include "device.h"

namespace adiscope {
class DeviceManager : public QObject
{
	Q_OBJECT
public:
	explicit DeviceManager(QObject *parent = nullptr);
	Device* getDevice(QString uri);
	void setExclusive(bool);
	bool getExclusive() const;

public Q_SLOTS:
	void addDevice(QString uri);
	void removeDevice(QString uri);

private Q_SLOTS:
	void changeToolListDevice();
	void connectDevice();
	void disconnectDevice();

Q_SIGNALS:
	void deviceChangedToolList(QString, QList<ToolMenuEntry>);
	void deviceAdded(QString, Device*);
	void deviceRemoved(QString);
	void deviceConnected(QString uri);
	void deviceDisconnected(QString uri);


private:
	bool exclusive = false;
	QList<QString> connectedDev;
	QMap<QString,Device*> map;

};
}

#endif // DEVICEMANAGER_H
