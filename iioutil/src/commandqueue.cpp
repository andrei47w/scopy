#include "commandqueue.h"

#include <QDebug>
#include <QtConcurrent/QtConcurrent>

#include <functional>

using namespace std;
using namespace scopy;

Q_LOGGING_CATEGORY(CAT_COMMANDQUEUE, "CommandQueue");

CommandQueue::CommandQueue(int numberOfThreads, QObject *parent)
	: QObject(parent)
	, m_running(false)
	, m_nbThreads(numberOfThreads)
{
	m_commandExecThreadPool.setMaxThreadCount(std::min(m_nbThreads, QThread::idealThreadCount()));
}

CommandQueue::~CommandQueue()
{
	requestStop();

	for(auto c : m_commandQueue) {
		delete c;
	}
	m_commandQueue.clear();
}

void CommandQueue::enqueue(Command *command)
{
	m_commandQueue.push_back(command);
	qDebug(CAT_COMMANDQUEUE) << "enqueued " << command;

	if(!m_running) {
		start();
	}
}

void CommandQueue::start()
{
	m_running = true;
	runCmd();
}

void CommandQueue::resolveNext(scopy::Command *cmd)
{
	m_commandQueue.pop_front(); // also delete/disconnect
	qDebug(CAT_COMMANDQUEUE) << "delete " << cmd;
	disconnect(cmd, &Command::finished, this, &CommandQueue::resolveNext);
	cmd->deleteLater();

	if(m_commandQueue.size() == 0) {
		m_running = false;

	} else {
		runCmd();
	}
}

void CommandQueue::runCmd()
{
	qDebug(CAT_COMMANDQUEUE) << "run cmd " << m_commandQueue.size();
	if(m_running) {
		connect(m_commandQueue.at(0), &Command::finished, this, &CommandQueue::resolveNext);
		QtConcurrent::run(&m_commandExecThreadPool, std::bind([=]() {
			qDebug(CAT_COMMANDQUEUE) << "execute start " << m_commandQueue.at(0);
			m_commandQueue.at(0)->execute();
			qDebug(CAT_COMMANDQUEUE) << "execute stop " << m_commandQueue.at(0);
		}));
	}
}

void CommandQueue::requestStop()
{
	qDebug(CAT_COMMANDQUEUE) << "request stop " << m_commandQueue.size();
	m_running = false;
}

void CommandQueue::wait() {}

#include "moc_commandqueue.cpp"
