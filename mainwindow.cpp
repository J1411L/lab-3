#include "mainwindow.h"
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QDataStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentShape(None), movingFigure(nullptr), connecting(false) {
    // Создание панели инструментов и добавление действий
    QToolBar *toolBar = addToolBar("Shapes");

    toolBar->setStyleSheet("QToolButton { border-radius: 10px; padding: 10px; }"
                           "QToolButton:pressed { background-color: #0078d7; border: 1px solid #005299; }");

    QAction *rectangleAction = toolBar->addAction("Rectangle");
    QAction *triangleAction = toolBar->addAction("Triangle");
    QAction *ellipseAction = toolBar->addAction("Ellipse");
    QAction *moveAction = toolBar->addAction("Move");
    QAction *deleteAction = toolBar->addAction("Delete");
    QAction *connectAction = toolBar->addAction("Connect");

    // Подключение сигналов действий к слотам класса MainWindow
    connect(rectangleAction, &QAction::triggered, this, &MainWindow::setRectangleMode);
    connect(triangleAction, &QAction::triggered, this, &MainWindow::setTriangleMode);
    connect(ellipseAction, &QAction::triggered, this, &MainWindow::setEllipseMode);
    connect(moveAction, &QAction::triggered, this, &MainWindow::setMoveMode);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::setDeleteMode);
    connect(connectAction, &QAction::triggered, this, &MainWindow::setConnectMode);

    // Добавление кнопки "Стереть все" на панель инструментов и подключение к слоту clearAll()
    QAction *clearAction = toolBar->addAction("Clear All");
    connect(clearAction, &QAction::triggered, this, &MainWindow::clearAll);

    // Создание меню файла и добавление действий сохранения и загрузки
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("Save", this, &MainWindow::saveToFile);
    fileMenu->addAction("Load", this, &MainWindow::loadFromFile);
    setMenuBar(menuBar);

    resize(800, 600);  // Установка начального размера окна
}

MainWindow::~MainWindow() {}

void MainWindow::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);

    // Отрисовка всех фигур
    for (const Figure &figure : figures) {
        switch (figure.shape) {
        case Rectangle:
            painter.drawRect(figure.rect);
            break;
        case Triangle: {
            QPoint points[3] = {
                QPoint(figure.rect.left(), figure.rect.bottom()),
                QPoint(figure.rect.right(), figure.rect.bottom()),
                QPoint(figure.rect.center().x(), figure.rect.top())
            };
            painter.drawPolygon(points, 3);
            break;
        }
        case Ellipse:
            painter.drawEllipse(figure.rect);
            break;
        default:
            break;
        }
    }

    // Отрисовка всех связей
    for (const auto &connection : connections) {
        painter.drawLine(connection.first, connection.second);
    }

    // Предварительная отрисовка фигуры в процессе рисования
    if (currentShape == Rectangle || currentShape == Triangle || currentShape == Ellipse) {
        QRect rect(startPoint, endPoint);
        switch (currentShape) {
        case Rectangle:
            painter.drawRect(rect);
            break;
        case Triangle: {
            QPoint points[3] = {
                QPoint(rect.left(), rect.bottom()),
                QPoint(rect.right(), rect.bottom()),
                QPoint(rect.center().x(), rect.top())
            };
            painter.drawPolygon(points, 3);
            break;
        }
        case Ellipse:
            painter.drawEllipse(rect);
            break;
        default:
            break;
        }
    }

    // Отрисовка линии при создании связи
    if (currentShape == Connect && connecting) {
        painter.drawLine(connectionStartPoint, endPoint);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    // Если нажата левая кнопка мыши
    if (event->button() == Qt::LeftButton) {
        // Сохраняем начальную точку нажатия
        startPoint = event->pos();

        // Проверяем текущий режим
        if (currentShape == Move) {
            // Поиск фигуры для перемещения
            movingFigure = nullptr;
            for (auto &figure : figures) {
                // Если фигура содержит точку нажатия
                if (figure.rect.contains(startPoint)) {
                    // Помечаем эту фигуру как перемещаемую
                    movingFigure = &figure;
                    // Сохраняем последнюю позицию мыши
                    lastMousePos = event->pos();
                    break;
                }
            }
        } else if (currentShape == Connect) {
            // Начало создания связи
            connecting = false;
            for (auto &figure : figures) {
                // Если фигура содержит точку нажатия
                if (figure.rect.contains(startPoint)) {
                    // Устанавливаем флаг, что создание связи началось
                    connecting = true;
                    // Сохраняем центральную точку фигуры как начало связи
                    connectionStartPoint = figure.rect.center();
                    break;
                }
            }
        } else if (currentShape == Delete) {
            // Удаление фигуры и ее связей
            QList<QPair<QPoint, QPoint>> toRemoveConnections;
            for (auto &figure : figures) {
                // Если фигура содержит точку нажатия
                if (figure.rect.contains(startPoint)) {
                    // Удаление связей, в которых участвует удаляемая фигура
                    for (auto it = connections.begin(); it != connections.end(); ) {
                        if (it->first == figure.rect.center() || it->second == figure.rect.center()) {
                            it = connections.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    // Удаление самой фигуры
                    figures.removeOne(figure);
                    updateGraph();
                    update();
                    break; // Выходим после удаления первой найденной фигуры
                }
            }
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (currentShape == Move && movingFigure) {
        // Перемещение фигуры
        QPoint delta = event->pos() - lastMousePos; //Вычисляет разницу между текущим положением мыши и последним записанным положением мыши ( lastMousePos).
        int index = figures.indexOf(*movingFigure);//Находит индекс перемещаемой фигуры в figures списке
        if (index != -1) {
            moveConnectedFigures(index, delta); //Вызывает moveConnectedFigures() функцию, передавая индекс фигуры и вычисленную дельту, для обновления позиций всех связанных фигур.
            lastMousePos = event->pos(); //Обновляет lastMousePos переменную с учетом текущего положения мыши.
            update();  // Перерисовка только после перемещения фигуры
        }
    } else if (currentShape == Connect && connecting) {
        // Обновление конечной точки связи
        endPoint = event->pos(); //Обновляет endPoin tпеременную с учетом текущего положения мыши.
        update();  // Перерисовка для обновления конечной точки
    } else {
        // Обновление конечной точки для создания фигуры
        endPoint = event->pos();
        update();  // Перерисовка для обновления конечной точки
    }

    // Закрепление начальной точки для создания связи
    if (currentShape == Connect && event->buttons() & Qt::LeftButton && !connecting)
    {
        // Проверяем, что текущая фигура - "Connect" и нажата левая кнопка мыши, но пользователь еще не начал процесс создания связи
        for (int i = 0; i < figures.size(); ++i)
        {
            // Итерируемся по списку фигур
            if (figures.at(i).rect.contains(event->pos()))
            {
                // Проверяем, что текущая позиция мыши находится внутри прямоугольника текущей фигуры
                connectionStartPoint = figures.at(i).rect.center(); // Устанавливаем начальную точку связи в центр прямоугольника фигуры
                connecting = true; // Устанавливаем флаг connecting в true, показывая, что пользователь начал процесс создания связи
                break; // Выходим из цикла, так как нашли начальную точку
            }
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    //функция сначала проверяет, отпустил ли пользователь левую кнопку мыши, проверяя event->button() значение.
    // Затем функция сохраняет текущее положение мыши ( event->pos())endPoint.
    if (event->button() == Qt::LeftButton) {
        endPoint = event->pos();
        QRect rect(startPoint, endPoint);
        //В зависимости от текущего currentShape значения функция выполняет различные действия:
        //addFigure()функция, передающая тип фигуры и прямоугольник, определяемые с помощью startPointи endPoint.
        switch (currentShape) {
        case Rectangle:
        case Triangle:
        case Ellipse:
            // Добавление новой фигуры
            addFigure({ currentShape, rect });
            break;
        case Connect:
            if (connecting) {
                // Завершение создания связи
                for (auto &figure : figures) {
                    if (figure.rect.contains(endPoint) && figure.rect.center() != connectionStartPoint) {
                        connections.append(qMakePair(connectionStartPoint, figure.rect.center()));
                        updateGraph();
                        break;
                    }
                }
                connecting = false;
            }
            break;
        default:
            break;
        }
        //После выполнения необходимых действийupdate()метод инициирования перерисовки главного окна приложения.
        update();
    }
    //функция устанавливаетmovingFigureуказатель на nullptr, указывающий на то, что в данный момент ни одна фигура не перемещается.
    movingFigure = nullptr;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        // Отмена текущего режима
        currentShape = None;
        movingFigure = nullptr;
        connecting = false;
        update();
    }
}

void MainWindow::setRectangleMode() {
    currentShape = Rectangle;
}

void MainWindow::setTriangleMode() {
    currentShape = Triangle;
}

void MainWindow::setEllipseMode() {
    currentShape = Ellipse;
}

void MainWindow::setMoveMode() {
    currentShape = Move;
}

void MainWindow::setDeleteMode() {
    currentShape = Delete;
}

void MainWindow::setConnectMode() {
    currentShape = Connect;
}

void MainWindow::saveToFile() {
    // 1. Открытие диалога сохранения файла
    QString fileName = QFileDialog::getSaveFileName(this, "Save File", "", "Text Files (*.txt)");

    // 2. Проверка, был ли выбран файл для сохранения
    if (!fileName.isEmpty()) {
        // 3. Создание объекта файла с выбранным именем
        QFile file(fileName);

        // 4. Открытие файла в режиме записи
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // 5. Создание объекта текстового потока для записи данных в файл
            QTextStream out(&file);

            // 6. Записываем количество фигур и количество связей
            out << "Figures: " << figures.size() << "\n";
            out << "Connections: " << connections.size() << "\n";

            // 7. Запись данных фигур в файл
            for (const auto& figure : figures) {
                QString shapeType;
                switch (figure.shape) {
                case Rectangle: shapeType = "Rectangle"; break;
                case Triangle: shapeType = "Triangle"; break;
                case Ellipse: shapeType = "Ellipse"; break;
                default: shapeType = "Unknown"; break;
                }
                out << shapeType << ": " << figure.rect.left() << " " << figure.rect.top() << " "
                    << figure.rect.width() << " " << figure.rect.height() << "\n";
            }

            // 8. Запись данных связей в файл
            for (const auto& connection : connections) {
                out << "Connection: " << connection.first.x() << " " << connection.first.y() << " "
                    << connection.second.x() << " " << connection.second.y() << "\n";
            }

            // 9. Закрытие файла
            file.close();

            qDebug() << "Файл успешно сохранен и закрыт.";
        } else {
            qDebug() << "Ошибка: не удалось открыть файл для записи";
        }
    } else {
        qDebug() << "Ошибка: не выбран файл для сохранения";
    }
}

void MainWindow::loadFromFile() {
    // 1. Открытие диалога выбора файла для загрузки
    QString fileName = QFileDialog::getOpenFileName(this, "Load File", "", "Text Files (*.txt)");

    // 2. Проверка, был ли выбран файл для загрузки
    if (!fileName.isEmpty()) {
        // 3. Создание объекта файла с выбранным именем
        QFile file(fileName);

        // 4. Открытие файла в режиме чтения
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 5. Создание объекта текстового потока для чтения данных из файла
            QTextStream in(&file);

            // 6. Считываем количество фигур и количество связей
            QString line = in.readLine();
            int figureCount = 0;
            int connectionCount = 0;

            if (line.startsWith("Figures: ")) {
                figureCount = line.mid(QString("Figures: ").length()).toInt();
            }

            line = in.readLine();
            if (line.startsWith("Connections: ")) {
                connectionCount = line.mid(QString("Connections: ").length()).toInt();
            }

            qDebug() << "Загружено фигур: " << figureCount;
            qDebug() << "Загружено связей: " << connectionCount;

            figures.clear();
            connections.clear();

            // 7. Чтение данных фигур из файла
            for (int i = 0; i < figureCount; ++i) {
                line = in.readLine();
                QStringList parts = line.split(": ");
                if (parts.size() == 2) {
                    QString shapeType = parts[0];
                    QStringList rectParts = parts[1].split(" ");
                    if (rectParts.size() == 4) {
                        QRect rect(rectParts[0].toInt(), rectParts[1].toInt(),
                                   rectParts[2].toInt(), rectParts[3].toInt());
                        Shape shape = None;
                        if (shapeType == "Rectangle") shape = Rectangle;
                        else if (shapeType == "Triangle") shape = Triangle;
                        else if (shapeType == "Ellipse") shape = Ellipse;

                        if (shape != None) {
                            figures.append({ shape, rect });
                        }
                    }
                }
            }

            // 8. Чтение данных связей из файла
            for (int i = 0; i < connectionCount; ++i) {
                line = in.readLine();
                QStringList parts = line.split(": ");
                if (parts.size() == 2 && parts[0] == "Connection") {
                    QStringList pointParts = parts[1].split(" ");
                    if (pointParts.size() == 4) {
                        QPoint point1(pointParts[0].toInt(), pointParts[1].toInt());
                        QPoint point2(pointParts[2].toInt(), pointParts[3].toInt());
                        connections.append(qMakePair(point1, point2));
                    }
                }
            }

            updateGraph();
            update();

            // 9. Закрытие файла
            file.close();
        } else {
            qDebug() << "Ошибка: не удалось открыть файл для чтения";
        }
    } else {
        qDebug() << "Ошибка: не выбран файл для загрузки";
    }
}



QDataStream &operator<<(QDataStream &out, const Figure &figure) {
    // Запись типа фигуры
    out << static_cast<qint32>(figure.shape);

    // Запись размера данных фигуры
    qint32 dataSize = 0;
    QByteArray data;
    {
        QDataStream dataStream(&data, QIODevice::WriteOnly);
        dataStream << figure.rect;
        dataSize = data.size();
    }
    out << dataSize;

    // Запись данных фигуры
    out.writeRawData(data.constData(), dataSize);

    return out;
}

QDataStream &operator>>(QDataStream &in, Figure &figure) {
    // Чтение фигуры из потока данных
    int shape;
    in >> shape;
    figure.shape = static_cast<Shape>(shape);
    in >> figure.rect;
    return in;
}


void MainWindow::updateGraph() {
    // Обновление графа связей между фигурами
    graph.clear();
    //функция проходит по всем существующим связям (connections). Для каждой связи она пытается найти индексы фигуры, соответствующие началу и концу этой связи.
    /*Для каждой связи в connections функция ищет индексы соответствующих фигур в figures.
     * Это делается с помощью вложенного цикла, который перебирает все фигуры. Внутри цикла проверяется,
     * совпадают ли центры прямоугольников фигур (figures[j].rect.center()) с координатами начала и конца
     * текущей связи (connections[i].first и connections[i].second). Если совпадение найдено, то
     * соответствующие индексы фигур сохраняются в переменные startIdx и endIdx.*/
    for (int i = 0; i < connections.size(); ++i) {
        int startIdx = -1, endIdx = -1;
        for (int j = 0; j < figures.size(); ++j) {
            if (figures[j].rect.center() == connections[i].first) {
                startIdx = j;
            }
            if (figures[j].rect.center() == connections[i].second) {
                endIdx = j;
            }
        }

        /*После того, как для текущей связи были найдены индексы начальной и
         * конечной фигур, эти данные добавляются в граф graph. Это происходит следующим образом:
         * Индекс конечной фигуры (endIdx) вставляется в множество, связанное с индексом начальной
         * фигуры (graph[startIdx].insert(endIdx)). Это означает, что начальная фигура связана с конечной.
         * Индекс начальной фигуры (startIdx) вставляется в множество, связанное с индексом конечной
         * фигуры (graph[endIdx].insert(startIdx)). Это означает, что конечная фигура связана с начальной.*/
        if (startIdx != -1 && endIdx != -1) {
            graph[startIdx].insert(endIdx);
            graph[endIdx].insert(startIdx);
        }
    }
}

void MainWindow::moveConnectedFigures(int index, const QPoint &delta) {
    // Перемещение выбранной фигуры
    /*ункция принимает indexпараметр, представляющий индекс выбранной фигуры в figures списке.
     * Затем она обновляет rectсвойство выбранной фигуры, перемещая ее верхний левый угол
     * на указанное значение delta(величина перемещения).*/
    figures[index].rect.moveTopLeft(figures[index].rect.topLeft() + delta);

    // Обновление позиций связей, связанных с перемещаемой фигурой
    /*Затем функция выполняет итерацию по connectionsсписку, содержащему начальные и
     *  конечные точки соединений между фигурами. Для каждого соединения она проверяет,
     *  содержится ли начальная или конечная точка соединения в обновленном прямоугольнике
     *  выбранной фигуры. Если да, она обновляет соответствующую точку соединения, чтобы
     *  она стала центром прямоугольника выбранной фигуры. Это гарантирует, что при перемещении
     *  выбранной фигуры присоединенные к ней соединения также обновляются, чтобы сохранить
     *  визуальное представление соединения.*/
    for (auto &connection : connections) {
        // Обновляем только концы линий, привязанные к перемещаемой фигуре
        if (figures[index].rect.contains(connection.first)) {
            connection.first = figures[index].rect.center();
        } else if (figures[index].rect.contains(connection.second)) {
            connection.second = figures[index].rect.center();
        }
    }

    // Инициация перерисовки окна
    /*После обновления фигуры и связанных с ней соединений функция вызывает метод update()
         * для инициирования перерисовки главного окна приложения. Это гарантирует, что изменения,
         * внесенные в фигуру и соединения, немедленно отразятся в визуальном представлении приложения.*/
    update();
}

void MainWindow::addFigure(const Figure &figure) {
    // Добавление новой фигуры в список фигур
    figures.append(figure);
    updateGraph();
}

void MainWindow::clearAll() {
    // Очистка всех фигур и связей
    figures.clear();
    connections.clear();
    graph.clear();
    update();
}
