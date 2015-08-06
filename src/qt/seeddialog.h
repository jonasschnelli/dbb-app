#ifndef SEEDDIALOG_H
#define SEEDDIALOG_H

#include <QDialog>

namespace Ui
{
class SeedDialog;
}

class SeedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SeedDialog(QWidget* parent = 0);
    ~SeedDialog();
    int SelectedWalletType();

private:
    Ui::SeedDialog* ui;
};

#endif // SEEDDIALOG_H
