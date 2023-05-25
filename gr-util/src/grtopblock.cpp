#include "grtopblock.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(TOPBLOCK, "GRTopBlock")

using namespace scopy::grutil;
GRTopBlock::GRTopBlock(QString name, QObject *parent) : running(false), built(false)
{

}

GRTopBlock::~GRTopBlock()
{

}

GRSignalPath *GRTopBlock::addSignalPath(QString name) {
	GRSignalPath* sig = new GRSignalPath(name, this);
	m_signalPaths.append(sig);
	QObject::connect(sig,SIGNAL(requestRebuild()),this,SLOT(rebuild()));
	return sig;
}

void GRTopBlock::registerIIODeviceSource(GRIIODeviceSource *dev)
{
	if(m_iioDeviceSources.contains(dev))
		return;
	m_iioDeviceSources.append(dev);
}

void GRTopBlock::unregisterIIODeviceSource(GRIIODeviceSource *dev)
{
	m_iioDeviceSources.removeAll(dev);
}

void GRTopBlock::build() {
	top = gr::make_top_block(m_name.toStdString());

	for (GRSignalPath* sig : qAsConst(m_signalPaths)) {
		if(sig->enabled() ) {
			sig->connect_blk(this, nullptr);
		}
	}
	for (GRIIODeviceSource* dev : qAsConst(m_iioDeviceSources)) {
		dev->build_blks(this);
		dev->connect_blk(this, nullptr);
	}
	Q_EMIT builtSignalPaths();
	built = true;
}

void GRTopBlock::teardown() {
	built = false;

	for (GRIIODeviceSource* dev : qAsConst(m_iioDeviceSources)) {
		dev->disconnect_blk(this);
		dev->destroy_blks(this);
	}

	for(GRSignalPath* sig : qAsConst(m_signalPaths)) {
		sig->disconnect_blk(this);
	}

	top->disconnect_all();
	Q_EMIT teardownSignalPaths();
}

void GRTopBlock::start()
{
	running = true;
	top->start();
}

void GRTopBlock::stop()
{
	running = false;
	top->stop();
	top->wait(); // ??
}

void GRTopBlock::run()
{
	start();
	top->wait();
}

void GRTopBlock::rebuild() {
	bool wasRunning = false;
	if(running) {
		wasRunning = true;
		stop();
	}

	if(built) {
		teardown();
		build();
	}

	if(wasRunning) {
		start();
	}
}

void GRTopBlock::connect(gr::basic_block_sptr src, int srcPort, gr::basic_block_sptr dst, int dstPort)
{
	qDebug(TOPBLOCK) << "Connecting " << QString::fromStdString(src->symbol_name()) << ":" << srcPort
					 << "to" << QString::fromStdString(dst->symbol_name()) << ":" << dstPort;
	top->connect(src,srcPort,dst,dstPort);
}

gr::top_block_sptr GRTopBlock::getGrBlock() { return top; }
