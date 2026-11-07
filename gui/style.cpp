#include "style.h"

namespace taskui {

const char* appStyleSheet() {
    return R"QSS(
QWidget {
    background: #f5f7fb;
    color: #24324a;
    font-family: "Microsoft YaHei";
    font-size: 14px;
}
QLabel#appTitle {
    color: #173b72;
    font-size: 25px;
    font-weight: 700;
}
QLabel#pageTitle {
    color: #173b72;
    font-size: 20px;
    font-weight: 700;
}
QLabel#pageDescription, QLabel#hint {
    color: #6a7890;
}
QFrame#card {
    background: white;
    border: 1px solid #e1e7f0;
    border-radius: 10px;
}
QLabel#summaryCard {
    background: #eef5ff;
    border: 1px solid #cfe0fa;
    border-radius: 8px;
    color: #1c467f;
    font-size: 15px;
    font-weight: 600;
    padding: 12px;
}
QLineEdit {
    background: white;
    border: 1px solid #cfd8e6;
    border-radius: 7px;
    padding: 8px 10px;
    selection-background-color: #2c78d4;
}
QLineEdit:focus {
    border: 2px solid #4d91e7;
}
QPushButton {
    background: #2f7fd8;
    border: 0;
    border-radius: 7px;
    color: white;
    font-weight: 600;
    padding: 8px 16px;
}
QPushButton:hover {
    background: #2169b8;
}
QPushButton:disabled {
    background: #aab8c9;
}
QPushButton#dangerButton {
    background: #c9463d;
}
QPushButton#dangerButton:hover {
    background: #a9362f;
}
QTabWidget::pane {
    background: white;
    border: 1px solid #dfe6f0;
    border-radius: 10px;
    top: -1px;
}
QTabBar::tab {
    background: #e9eef6;
    border: 0;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
    color: #52637c;
    margin-right: 4px;
    padding: 10px 20px;
}
QTabBar::tab:selected {
    background: white;
    color: #1d5fae;
    font-weight: 700;
}
QHeaderView::section {
    background: #edf3fb;
    border: 0;
    border-bottom: 1px solid #dbe5f1;
    color: #42516a;
    font-weight: 700;
    padding: 9px;
}
QTableView, QTableWidget {
    alternate-background-color: #f8faff;
    background: white;
    border: 1px solid #dfe6f0;
    border-radius: 8px;
    gridline-color: #edf1f6;
    selection-background-color: #d9eafe;
    selection-color: #1b3d69;
}
QCheckBox {
    color: #52637c;
    spacing: 7px;
}
)QSS";
}

} // namespace taskui
