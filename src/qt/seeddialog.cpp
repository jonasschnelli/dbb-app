#include "seeddialog.h"
#include "ui/ui_seeddialog.h"

SeedDialog::SeedDialog(QWidget* parent) : QDialog(parent),
                                          ui(new Ui::SeedDialog)
{
    ui->setupUi(this);
}

SeedDialog::~SeedDialog()
{
    delete ui;
}

int SeedDialog::SelectedWalletType()
{
    return ui->comboBox->currentIndex();
}
