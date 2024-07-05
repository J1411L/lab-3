// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QFileDialog>
#include <QFile>
#include <QDataStream>
#include <QAction>
#include <QMenuBar>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QList>
#include <QVBoxLayout>


// Перечисление форм
enum Shape { None, Rectangle, Triangle, Ellipse, Line, Move, Delete, Connect };

// Структура, представляющая фигуру
struct Figure {
    Shape shape;
    QRect rect;

    // Оператор сравнения
    bool operator==(const Figure &other) const {
        return shape == other.shape && rect == other.rect;
    }
};


// Перегрузка операторов потокового ввода/вывода для структуры Figure
QDataStream &operator<<(QDataStream &out, const Figure &figure);
QDataStream &operator>>(QDataStream &in, Figure &figure);

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void setRectangleMode();
    void setTriangleMode();
    void setEllipseMode();
    void setMoveMode();
    void setDeleteMode();
    void setConnectMode();
    void saveToFile();
    void loadFromFile();
    void clearAll(); // Новый слот для очистки всех фигур

private:
    Shape currentShape;
    QPoint startPoint, endPoint;
    QVector<Figure> figures;
    QVector<QPair<QPoint, QPoint>> connections;
    Figure *movingFigure;
    QPoint lastMousePos;
    bool connecting;
    QPoint connectionStartPoint;

    void moveConnectedFigures(int index, const QPoint &delta);
    void updateGraph();
    void addFigure(const Figure &figure);


     QMap<int, QSet<int>> graph;  // Граф связей между фигурами
};

#endif // MAINWINDOW_H
