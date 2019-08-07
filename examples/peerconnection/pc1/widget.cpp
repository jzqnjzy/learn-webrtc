#include <QMessageBox>

#include "widget.h"
#include "ui_widget.h"

#include "peerconnectiona.h"
#include "peerconnectionb.h"

// unityplugin
//SimplePeerConnection

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , peer_connection_a_(new rtc::RefCountedObject<PeerConnectionA>())
    , peer_connection_b_(new rtc::RefCountedObject<PeerConnectionB>())
{
    ui->setupUi(this);
    // 模拟服务器将a的offer发送给b
    connect(peer_connection_a_, &PeerConnectionA::CreateOffered,
            peer_connection_b_, &PeerConnectionB::OnRecvOffer, Qt::DirectConnection);
    connect(peer_connection_b_, &PeerConnectionB::CreateAnswered,
            peer_connection_a_, &PeerConnectionA::OnRecvAnswer, Qt::DirectConnection);

    connect(peer_connection_a_, &SimplePeerConnection::OnIceCandidated,
            peer_connection_b_, &SimplePeerConnection::SetIceCandidate, Qt::DirectConnection);
    connect(peer_connection_b_, &SimplePeerConnection::OnIceCandidated,
            peer_connection_a_, &SimplePeerConnection::SetIceCandidate, Qt::DirectConnection);

    connect(peer_connection_b_, &PeerConnectionB::AddTracked, this,
            [this](webrtc::VideoTrackInterface* vieo_brack){
        StartRemoteRenderer(vieo_brack);
    }, Qt::DirectConnection);

    ui->callBtn->setEnabled(false);
    ui->hangUpBtn->setEnabled(false);
}

Widget::~Widget()
{
    StopLocalRenderer();
    StopRemoteRenderer();
    delete ui;
}

void Widget::StartLocalRenderer(webrtc::VideoTrackInterface* local_video) {
    local_renderer_.reset(new VideoRenderer(local_video, ui->localLabel));
}

void Widget::StopLocalRenderer() {
    local_renderer_.reset();
}

void Widget::StartRemoteRenderer(webrtc::VideoTrackInterface *remote_video)
{
    remote_renderer_.reset(new VideoRenderer(remote_video, ui->remoteLabel));
}

void Widget::StopRemoteRenderer()
{
    remote_renderer_.reset();
}

void Widget::on_startBtn_clicked()
{
    peer_connection_a_->CreateTracks();
    StartLocalRenderer(peer_connection_a_->GetVideoTrack());

    ui->startBtn->setEnabled(false);
    ui->callBtn->setEnabled(true);
}

void Widget::on_callBtn_clicked()
{
    if (!peer_connection_a_->GetVideoTrack()) {
        QMessageBox::warning(this, "Error", "You should start first",
                             QMessageBox::Ok);
        return;
    }

    if (peer_connection_a_->GetPeerConnection()) {
        QMessageBox::warning(this, "Error", "We only support connecting to one peer at a time",
                             QMessageBox::Ok);
        return;
    }

    // create peer a
    if (!peer_connection_a_->CreatePeerConnection()) {
        QMessageBox::warning(this, "Error", "CreatePeerConnection failed",
                             QMessageBox::Ok);
        peer_connection_a_->DeletePeerConnection();
        return;
    }

    // create peer b
    if (!peer_connection_b_->CreatePeerConnection()) {
        QMessageBox::warning(this, "Error", "CreatePeerConnection failed",
                             QMessageBox::Ok);
        peer_connection_a_->DeletePeerConnection();
        return;
    }

    // peer a send offer
    peer_connection_a_->AddTracks();
    peer_connection_a_->CreateOffer();

    ui->callBtn->setEnabled(false);
    ui->hangUpBtn->setEnabled(true);
}

void Widget::on_hangUpBtn_clicked()
{
    peer_connection_a_->DeletePeerConnection();
    peer_connection_b_->DeletePeerConnection();

    ui->callBtn->setEnabled(true);
    ui->hangUpBtn->setEnabled(false);
}
