#include <QMainWindow>
#include <QWidget>

namespace Ui {
    class MainWindow;
}

class DBBDaemonGui : public QMainWindow
{
	Q_OBJECT
		
public:
	explicit DBBDaemonGui(QWidget *parent = 0);
	~DBBDaemonGui();
	
	private:
		Ui::MainWindow *ui;
		void changeConnectedState(bool state);
			
	public slots:
	    /** Set number of connections shown in the UI */
	    void eraseClicked();
};