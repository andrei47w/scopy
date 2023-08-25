#ifndef TUTORIALOVERLAY_H
#define TUTORIALOVERLAY_H

#include <QWidget>
#include "tintedoverlay.h"
#include "tutorialchapter.h"
#include "scopy-gui_export.h"

namespace Ui {
class Tutorial;
}

namespace scopy::gui {

class SCOPY_GUI_EXPORT TutorialOverlay : public QWidget
{
	Q_OBJECT
public:
	explicit TutorialOverlay(QWidget *parent = nullptr);
	~TutorialOverlay();

	TutorialChapter* addChapter(QList<QWidget*> subjects, QString description);
	TutorialChapter* addChapter(QWidget* subject, QString description);
	void addChapter(TutorialChapter* ch);

	const QString &getTitle() const;
	void setTitle(const QString &newTitle);

public Q_SLOTS:
	virtual void start();
	void next();
	void finish();
	void abort();

Q_SIGNALS:
	/**
	 * @brief Used to signal that the current tutorial was fully completed by the user, not exited early.
	 * */
	void finished();

	/**
	 * @brief Used to signal that the current tutorial was exited early, without completion, by the user.
	 * */
	void aborted();

private:
	void buildUi();
	bool eventFilter(QObject *watched, QEvent *event) override;

	QList<TutorialChapter*> chapter;
	QWidget *parent;
	TintedOverlay *overlay;
	QList<TintedOverlay*> highlights;
	QString title;
	int cnt;
	Ui::Tutorial *ui;
};
}


#endif // TUTORIALOVERLAY_H
