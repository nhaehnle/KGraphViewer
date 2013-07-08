/*

Simple example using the DotGraphModel and GraphScene classes

*/

#include "dotgraphmodel.h"
#include "dotgraphdelegate.h"
#include "graphscene.h"

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KLocale>
#include <KXmlGuiWindow>

#include <QGraphicsView>

using KGraphViewer::NodeIndex;
using KGraphViewer::EdgeIndex;

class MainWindow : public KXmlGuiWindow {
public:
    MainWindow();

    void makeGraph();

private:
    KGraphViewer::DotGraphModel * m_model;
    KGraphViewer::GraphScene * m_scene;
    QGraphicsView * m_view;
};

MainWindow::MainWindow()
{
    makeGraph();

    m_scene = new KGraphViewer::GraphScene(this);
    m_scene->setItemDelegate(new KGraphViewer::DotGraphDelegate(this));
    m_scene->setModel(m_model);

    m_view = new QGraphicsView(this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setScene(m_scene);

    setCentralWidget(m_view);
    setupGUI();
}

void MainWindow::makeGraph()
{
    m_model = new KGraphViewer::DotGraphModel(this);
    NodeIndex nodeA = m_model->addNode(NodeIndex());
    m_model->setNodeData(nodeA, Qt::DisplayRole, "Node A");

    for (int i = 0; i < 8; ++i) {
        NodeIndex nodeB = m_model->addNode(NodeIndex());
        m_model->setNodeData(nodeB, Qt::DisplayRole, QString("Node B%1").arg(i + 1));
        m_model->addEdge(nodeA, nodeB);
    }

    m_model->layout("dot");
}


static const char description[] = I18N_NOOP("A simple example use of kgraphviewerlib");
static const char version[] = I18N_NOOP("1.0.0");

int main(int argc, char **argv)
{
    KAboutData aboutData(
        "simple", 0, ki18n("Simple KGraphViewerLib Example"), version, ki18n(description),
        KAboutData::License_GPL, ki18n("(C) 2013 Nicolai Hähnle"), KLocalizedString(), 0, "nhaehnle@gmail.com");
    aboutData.addAuthor(ki18n("Nicolai Hähnle"), ki18n("Original Author"), "nhaehnle@gmail.com");
    KCmdLineArgs::init(argc, argv, &aboutData);

    KApplication app;
    MainWindow * mainWindow = new MainWindow;
    mainWindow->show();
    return app.exec();
}

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
