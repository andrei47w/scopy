#include "instrumentnotes.h"

#include "ui_instrumentnotes.h"

namespace scopy {
InstrumentNotes::InstrumentNotes(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::InstrumentNotes)
{
	ui->setupUi(this);
}

InstrumentNotes::~InstrumentNotes() { delete ui; }

QString InstrumentNotes::getNotes() { return ui->textEdit->toPlainText(); }
void InstrumentNotes::setNotes(QString str) { ui->textEdit->setPlainText(str); }

} // namespace scopy

#include "moc_instrumentnotes.cpp"
