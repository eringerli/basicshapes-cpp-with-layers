// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "scenemodifier.h"

#include <QGuiApplication>

#include <Qt3DCore/qentity.h>
#include <Qt3DRender/qcamera.h>
#include <Qt3DRender/qcameralens.h>

#include <QtGui/QScreen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QCommandLinkButton>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>

#include <Qt3DExtras/qtorusmesh.h>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qmesh.h>
#include <Qt3DRender/qpointlight.h>
#include <Qt3DRender/qrenderpass.h>
#include <Qt3DRender/qsceneloader.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qtexture.h>

#include <Qt3DCore/qaspectengine.h>
#include <Qt3DCore/qtransform.h>

#include <Qt3DExtras/qforwardrenderer.h>
#include <Qt3DRender/qrenderaspect.h>

#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/qfirstpersoncameracontroller.h>
#include <Qt3DExtras/qt3dwindow.h>

#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QDebugOverlay>
#include <Qt3DRender/QFrustumCulling>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QNoDraw>

#include <Qt3DRender/QLayer>

int
main( int argc, char** argv ) {
  QApplication            app( argc, argv );
  Qt3DExtras::Qt3DWindow* view = new Qt3DExtras::Qt3DWindow();
  view->defaultFrameGraph()->setClearColor( QColor( QRgb( 0x4d4d4f ) ) );
  QWidget* container  = QWidget::createWindowContainer( view );
  QSize    screenSize = view->screen()->size();
  container->setMinimumSize( QSize( 200, 100 ) );
  container->setMaximumSize( screenSize );

  QWidget*     widget  = new QWidget;
  QHBoxLayout* hLayout = new QHBoxLayout( widget );
  QVBoxLayout* vLayout = new QVBoxLayout();
  vLayout->setAlignment( Qt::AlignTop );
  hLayout->addWidget( container, 1 );
  hLayout->addLayout( vLayout );

  widget->setWindowTitle( QStringLiteral( "Basic shapes" ) );

  // Root entity
  Qt3DCore::QEntity* rootEntity = new Qt3DCore::QEntity();

  auto* backgroundEntity   = new Qt3DCore::QEntity( rootEntity );
  auto* middlegroundEntity = new Qt3DCore::QEntity( rootEntity );
  auto* foregroundEntity   = new Qt3DCore::QEntity( rootEntity );

  auto* backgroundRenderingLayer   = new Qt3DRender::QLayer( backgroundEntity );
  auto* middlegroundRenderingLayer = new Qt3DRender::QLayer( middlegroundEntity );
  auto* foregroundRenderingLayer   = new Qt3DRender::QLayer( foregroundEntity );

  backgroundRenderingLayer->setRecursive( true );
  middlegroundRenderingLayer->setRecursive( true );
  foregroundRenderingLayer->setRecursive( true );

  backgroundEntity->addComponent( backgroundRenderingLayer );
  middlegroundEntity->addComponent( middlegroundRenderingLayer );
  foregroundEntity->addComponent( foregroundRenderingLayer );

  // Camera
  Qt3DRender::QCamera* cameraEntity = view->camera();

  cameraEntity->lens()->setPerspectiveProjection( 45.0f, 16.0f / 9.0f, 0.1f, 1000.0f );
  cameraEntity->setPosition( QVector3D( 0, 0, 20.0f ) );
  cameraEntity->setUpVector( QVector3D( 0, 1, 0 ) );
  cameraEntity->setViewCenter( QVector3D( 0, 0, 0 ) );

  Qt3DCore::QEntity*       lightEntity = new Qt3DCore::QEntity( rootEntity );
  Qt3DRender::QPointLight* light       = new Qt3DRender::QPointLight( lightEntity );
  light->setColor( "white" );
  light->setIntensity( 1 );
  lightEntity->addComponent( light );
  Qt3DCore::QTransform* lightTransform = new Qt3DCore::QTransform( lightEntity );
  lightTransform->setTranslation( cameraEntity->position() );
  lightEntity->addComponent( lightTransform );

  // For camera controls
  auto* camController = new Qt3DExtras::QOrbitCameraController( rootEntity );
  camController->setCamera( cameraEntity );

  // Scenemodifier
  SceneModifier* modifier = new SceneModifier( foregroundEntity, middlegroundEntity, backgroundEntity );

  // Set root object of the scene
  view->setRootEntity( rootEntity );

  //    view->defaultFrameGraph()->setShowDebugOverlay(true);

  // Create control widgets
  QCommandLinkButton* info = new QCommandLinkButton();
  info->setText( QStringLiteral( "Qt3D ready-made meshes" ) );
  info->setDescription( QString::fromLatin1( "Qt3D provides several ready-made meshes, like torus, cylinder, cone, "
                                             "cube, plane and sphere." ) );
  info->setIconSize( QSize( 0, 0 ) );

  // get the varioud children of the default framegraph
  auto*                        lastChildOfFrameGraph = view->activeFrameGraph();
  Qt3DRender::QFrameGraphNode *cameraSelector, *clearBuffers, *frustumCulling, *debugOverlay;

  while( true ) {
    if( !lastChildOfFrameGraph->children().empty() ) {
      lastChildOfFrameGraph = static_cast< Qt3DRender::QFrameGraphNode* >( lastChildOfFrameGraph->children().front() );

      if( qobject_cast< Qt3DRender::QCameraSelector* >( lastChildOfFrameGraph ) != nullptr ) {
        cameraSelector = lastChildOfFrameGraph;
      }
      if( qobject_cast< Qt3DRender::QClearBuffers* >( lastChildOfFrameGraph ) != nullptr ) {
        clearBuffers = lastChildOfFrameGraph;
      }
      if( qobject_cast< Qt3DRender::QFrustumCulling* >( lastChildOfFrameGraph ) != nullptr ) {
        frustumCulling = lastChildOfFrameGraph;
      }
      if( qobject_cast< Qt3DRender::QDebugOverlay* >( lastChildOfFrameGraph ) != nullptr ) {
        debugOverlay = lastChildOfFrameGraph;
      }

    } else {
      break;
    }
  }

  qDebug() << "Before changeing the Framegraph";
  view->activeFrameGraph()->dumpObjectTree();

  constexpr auto clearBufferType = Qt3DRender::QClearBuffers::DepthStencilBuffer;

  Qt3DRender::QNoDraw*       noDrawClearBuffers;
  Qt3DRender::QClearBuffers* clearBuffersForBackground;
  Qt3DRender::QLayerFilter*  backgroundRenderingLayerFilter;
  Qt3DRender::QClearBuffers* clearBuffersForMiddleground;
  Qt3DRender::QLayerFilter*  middlegroundRenderingLayerFilter;
  Qt3DRender::QClearBuffers* clearBuffersForForeground;
  Qt3DRender::QLayerFilter*  foregroundRenderingLayerFilter;

  //    debugOverlay->setParent(foregroundRenderingLayerFilter);

  qDebug() << "";
  qDebug() << "After changeing the Framegraph";
  view->activeFrameGraph()->dumpObjectTree();

  QCheckBox* torusCB = new QCheckBox( widget );
  torusCB->setChecked( true );
  torusCB->setText( QStringLiteral( "Torus (foreground)" ) );

  QCheckBox* coneCB = new QCheckBox( widget );
  coneCB->setChecked( true );
  coneCB->setText( QStringLiteral( "Cone (foreground)" ) );

  QCheckBox* cylinderCB = new QCheckBox( widget );
  cylinderCB->setChecked( true );
  cylinderCB->setText( QStringLiteral( "Cylinder (middleground)" ) );

  QCheckBox* cuboidCB = new QCheckBox( widget );
  cuboidCB->setChecked( true );
  cuboidCB->setText( QStringLiteral( "Cuboid (middleground)" ) );

  QCheckBox* planeCB = new QCheckBox( widget );
  planeCB->setChecked( true );
  planeCB->setText( QStringLiteral( "Plane (background)" ) );

  QCheckBox* sphereCB = new QCheckBox( widget );
  sphereCB->setChecked( true );
  sphereCB->setText( QStringLiteral( "Sphere (background)" ) );

  QCheckBox* layerFilterCB = new QCheckBox( widget );
  layerFilterCB->setChecked( false );
  layerFilterCB->setText( QStringLiteral( "Layer Filter" ) );

  vLayout->addWidget( info );
  vLayout->addWidget( torusCB );
  vLayout->addWidget( coneCB );
  vLayout->addWidget( cylinderCB );
  vLayout->addWidget( cuboidCB );
  vLayout->addWidget( planeCB );
  vLayout->addWidget( sphereCB );
  vLayout->addWidget( layerFilterCB );

  QObject::connect( torusCB, &QCheckBox::stateChanged, modifier, &SceneModifier::enableTorus );
  QObject::connect( coneCB, &QCheckBox::stateChanged, modifier, &SceneModifier::enableCone );
  QObject::connect( cylinderCB, &QCheckBox::stateChanged, modifier, &SceneModifier::enableCylinder );
  QObject::connect( cuboidCB, &QCheckBox::stateChanged, modifier, &SceneModifier::enableCuboid );
  QObject::connect( planeCB, &QCheckBox::stateChanged, modifier, &SceneModifier::enablePlane );
  QObject::connect( sphereCB, &QCheckBox::stateChanged, modifier, &SceneModifier::enableSphere );

  QObject::connect( layerFilterCB, &QCheckBox::stateChanged, [&]( bool state ) {
    qDebug() << "";
    qDebug() << "Before changeing the Framegraph";
    view->activeFrameGraph()->dumpObjectTree();

    if( state ) {
      noDrawClearBuffers = new Qt3DRender::QNoDraw( clearBuffers );
      frustumCulling->setParent( clearBuffers );
      frustumCulling->setParent( cameraSelector );

      clearBuffersForBackground = new Qt3DRender::QClearBuffers( frustumCulling );
      clearBuffersForBackground->setBuffers( clearBufferType );
      backgroundRenderingLayerFilter = new Qt3DRender::QLayerFilter( clearBuffersForBackground );
      backgroundRenderingLayerFilter->addLayer( foregroundRenderingLayer );
      backgroundRenderingLayerFilter->addLayer( middlegroundRenderingLayer );
      backgroundRenderingLayerFilter->setFilterMode( Qt3DRender::QLayerFilter::FilterMode::DiscardAnyMatchingLayers );

      clearBuffersForMiddleground = new Qt3DRender::QClearBuffers( frustumCulling );
      clearBuffersForMiddleground->setBuffers( clearBufferType );
      middlegroundRenderingLayerFilter = new Qt3DRender::QLayerFilter( clearBuffersForMiddleground );
      middlegroundRenderingLayerFilter->addLayer( foregroundRenderingLayer );
      middlegroundRenderingLayerFilter->addLayer( backgroundRenderingLayer );
      middlegroundRenderingLayerFilter->setFilterMode( Qt3DRender::QLayerFilter::FilterMode::DiscardAnyMatchingLayers );

      clearBuffersForForeground = new Qt3DRender::QClearBuffers( frustumCulling );
      clearBuffersForForeground->setBuffers( clearBufferType );
      foregroundRenderingLayerFilter = new Qt3DRender::QLayerFilter( clearBuffersForForeground );
      foregroundRenderingLayerFilter->addLayer( middlegroundRenderingLayer );
      foregroundRenderingLayerFilter->addLayer( backgroundRenderingLayer );
      foregroundRenderingLayerFilter->setFilterMode( Qt3DRender::QLayerFilter::FilterMode::DiscardAnyMatchingLayers );

      debugOverlay->setParent( foregroundRenderingLayerFilter );
    } else {
      frustumCulling->setParent( clearBuffers );
      debugOverlay->setParent( frustumCulling );

      noDrawClearBuffers->deleteLater();
      clearBuffersForForeground->deleteLater();
      clearBuffersForMiddleground->deleteLater();
      clearBuffersForBackground->deleteLater();
      foregroundRenderingLayerFilter->deleteLater();
      middlegroundRenderingLayerFilter->deleteLater();
      backgroundRenderingLayerFilter->deleteLater();
    }

    qDebug() << "";
    qDebug() << "After changeing the Framegraph";
    view->activeFrameGraph()->dumpObjectTree();
  } );

  torusCB->setChecked( true );
  coneCB->setChecked( true );
  cylinderCB->setChecked( true );
  cuboidCB->setChecked( true );
  planeCB->setChecked( true );
  sphereCB->setChecked( true );

  // Show window
  widget->show();
  widget->resize( 1200, 800 );

  return app.exec();
}
