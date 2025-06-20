#include "mainwindow.h"
#include "frame_desenho.h"
#include "ui_mainwindow.h"

// Modelos e Utilitários
#include "display_file.h"
#include "camera.h"
#include "objeto_grafico.h"
#include "ponto3d.h"
#include "transformador_geometrico.h"
#include "gerenciadorobjetosdialog.h"
#include "carregador_obj.h"
#include "malha_obj.h"

// Módulos Qt
#include <QColorDialog>
#include <QMessageBox>
#include <QDebug>
#include <QRadioButton>
#include <QFileDialog>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    inicializarComponentes();
    conectarSinais();
    inicializarUI();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::inicializarComponentes() {
    displayFile = std::make_shared<DisplayFile>();
    popularDisplayFileInicial();

    if (ui->frameDesenho) {
        ui->frameDesenho->definirDisplayFile(displayFile);
    } else {
        QMessageBox::critical(this, "Erro de UI", "O 'frameDesenho' não foi carregado corretamente.");
    }

    objetoSelecionado = nullptr;
    atualizarCbDisplayFile();
    atualizarCbDFCamera();

    if (ui->cbDFCameras->count() > 0) {
        ui->cbDFCameras->setCurrentIndex(0);
    }

    if (ui->frameDesenho) {
        ui->frameDesenho->redesenhar();
    }
}

void MainWindow::conectarSinais() {
    // --- CONEXÕES DE SELEÇÃO ---
    connect(ui->cbDisplayFile, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_cbDisplayFile_currentIndexChanged);
    connect(ui->cbDFCameras, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_cbDFCamera_currentIndexChanged);

    // --- CONEXÕES DE TRANSFORMAÇÃO 3D ---
    connect(ui->spinTranslacaoX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::aplicarTranslacaoAtual);
    connect(ui->spinTranslacaoY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::aplicarTranslacaoAtual);
    connect(ui->spinTranslacaoZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::aplicarTranslacaoAtual);

    connect(ui->spinEscalaX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::aplicarEscalaAtual);
    connect(ui->spinEscalaY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::aplicarEscalaAtual);
    connect(ui->spinEscalaZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::aplicarEscalaAtual);

    connect(ui->hsRotacaoX, &QSlider::valueChanged, this, &MainWindow::aplicarRotacaoAtual);
    connect(ui->hsRotacaoY, &QSlider::valueChanged, this, &MainWindow::aplicarRotacaoAtual);
    connect(ui->hsRotacaoZ, &QSlider::valueChanged, this, &MainWindow::aplicarRotacaoAtual);
}

void MainWindow::inicializarUI() {
    resetarControlesTransformacao();
    if(ui->frameDesenho) {
        ui->frameDesenho->setFocusPolicy(Qt::StrongFocus);
    }
    ui->rbTransformarCamera->setChecked(true);
}

void MainWindow::resetarControlesTransformacao() {
    // Bloqueia sinais para evitar disparos desnecessários ao resetar os valores
    ui->spinEscalaX->blockSignals(true);
    ui->spinEscalaY->blockSignals(true);
    ui->spinEscalaZ->blockSignals(true);
    ui->hsRotacaoX->blockSignals(true);
    ui->hsRotacaoY->blockSignals(true);
    ui->hsRotacaoZ->blockSignals(true);
    ui->spinTranslacaoX->blockSignals(true);
    ui->spinTranslacaoY->blockSignals(true);
    ui->spinTranslacaoZ->blockSignals(true);

    ui->spinEscalaX->setValue(0);
    ui->spinEscalaY->setValue(0);
    ui->spinEscalaZ->setValue(0);
    ui->hsRotacaoX->setValue(0);
    ui->hsRotacaoY->setValue(0);
    ui->hsRotacaoZ->setValue(0);
    ui->spinTranslacaoX->setValue(0);
    ui->spinTranslacaoY->setValue(0);
    ui->spinTranslacaoZ->setValue(0);

    // Desbloqueia os sinais
    ui->spinEscalaX->blockSignals(false);
    ui->spinEscalaY->blockSignals(false);
    ui->spinEscalaZ->blockSignals(false);
    ui->hsRotacaoX->blockSignals(false);
    ui->hsRotacaoY->blockSignals(false);
    ui->hsRotacaoZ->blockSignals(false);
    ui->spinTranslacaoX->blockSignals(false);
    ui->spinTranslacaoY->blockSignals(false);
    ui->spinTranslacaoZ->blockSignals(false);
}

void MainWindow::popularDisplayFileInicial()
{
    if (displayFile) {
        // Cria uma câmera padrão
        auto cameraPadrao = std::make_shared<Camera>(
            Ponto3D(0, 5, 15),
            Ponto3D(0, 0, 0),
            "Câmera Padrão"
            );
        displayFile->adicionarCamera(cameraPadrao);
        displayFile->definirCameraAtiva(cameraPadrao);
    }
}

void MainWindow::atualizarCbDisplayFile() {
    ui->cbDisplayFile->blockSignals(true);
    QString nomeObjetoSelecionadoAnteriormente = objetoSelecionado ? objetoSelecionado->obterNome() : "";
    ui->cbDisplayFile->clear();

    if (displayFile) {
        for (const auto& obj : displayFile->obterObjetos()) {
            if (obj) {
                // Supondo que gerarNomeFormatadoParaObjeto exista e esteja correto
                ui->cbDisplayFile->addItem(obj->obterNome(), QVariant::fromValue(obj->obterNome()));
            }
        }
    }

    if (!nomeObjetoSelecionadoAnteriormente.isEmpty()) {
        int idx = ui->cbDisplayFile->findData(nomeObjetoSelecionadoAnteriormente);
        if (idx != -1) {
            ui->cbDisplayFile->setCurrentIndex(idx);
        } else {
            objetoSelecionado = nullptr;
        }
    }
    ui->cbDisplayFile->blockSignals(false);
    updateTransformationTargetUIState();
}

void MainWindow::atualizarCbDFCamera() {
    ui->cbDFCameras->blockSignals(true);
    QString nomeCameraAtivaAnteriormente;
    if (displayFile && displayFile->obterCameraAtiva()) {
        nomeCameraAtivaAnteriormente = displayFile->obterCameraAtiva()->obterNome();
    }

    ui->cbDFCameras->clear();
    if (displayFile) {
        for (const auto& cam : displayFile->obterListaCameras()) {
            if (cam) {
                ui->cbDFCameras->addItem(cam->obterNome(), QVariant::fromValue(cam->obterNome()));
            }
        }
    }

    if (!nomeCameraAtivaAnteriormente.isEmpty()) {
        int idx = ui->cbDFCameras->findData(nomeCameraAtivaAnteriormente);
        if (idx != -1) {
            ui->cbDFCameras->setCurrentIndex(idx);
        }
    }
    ui->cbDFCameras->blockSignals(false);
}

void MainWindow::on_cbDFCamera_currentIndexChanged(int index) {
    if (!displayFile || index < 0) {
        if (displayFile) {
            displayFile->definirCameraAtiva(std::shared_ptr<Camera>());
        }
        updateTransformationTargetUIState();
        return;
    }

    QString nomeCamera = ui->cbDFCameras->itemData(index).toString();
    auto cameraSelecionada = displayFile->buscarCamera(nomeCamera);

    if (cameraSelecionada) {
        ui->rbTransformarCamera->setChecked(true);

        displayFile->definirCameraAtiva(cameraSelecionada);

        if (objetoSelecionado) {
            ui->cbDisplayFile->setCurrentIndex(-1);
        }
        if (ui->frameDesenho) {
            ui->frameDesenho->redesenhar();
        }
    } else {
        qWarning() << "Câmera não encontrada:" << nomeCamera;
        displayFile->definirCameraAtiva(std::shared_ptr<Camera>());
    }
    updateTransformationTargetUIState();
}

void MainWindow::on_cbDisplayFile_currentIndexChanged(int index)
{
    // Se nenhum item estiver selecionado (índice -1), limpa o ponteiro e sai.
    if (!displayFile || index < 0) {
        objetoSelecionado = nullptr;
        updateTransformationTargetUIState();
        return;
    }

    // Busca o objeto pelo nome armazenado no ComboBox.
    QString nomeObjeto = ui->cbDisplayFile->itemData(index).toString();
    objetoSelecionado = displayFile->buscarObjeto(nomeObjeto);

    // ESTE 'IF' É CRUCIAL:
    // Somente se o objeto foi encontrado com sucesso (ponteiro não é nulo)...
    if (objetoSelecionado) {
        // ...então, define a intenção da UI para transformar o objeto.
        ui->rbTransformarObjeto->setChecked(true);

        // E desmarca a seleção da câmera para evitar confusão.
        if (ui->cbDFCameras->currentIndex() != -1) {
            ui->cbDFCameras->setCurrentIndex(-1);
        }
    } else {
        // Caso o objeto não seja encontrado por algum motivo, avisa sobre o erro.
        qWarning() << "LÓGICA DE SELEÇÃO FALHOU: Objeto '" << nomeObjeto << "' não encontrado no DisplayFile!";
    }

    // Atualiza o estado geral da UI.
    updateTransformationTargetUIState();
}


// --- LÓGICA DE TRANSFORMAÇÃO ---

void MainWindow::aplicarTranslacaoAtual() {
    if (!ui->tabWidget->isEnabled()) {
        return;
    }

    double dx = ui->spinTranslacaoX->value();
    double dy = ui->spinTranslacaoY->value();
    double dz = ui->spinTranslacaoZ->value();
    if (dx == 0 && dy == 0 && dz == 0) return;


    auto camera = displayFile->obterCameraAtiva();
    if (ui->rbTransformarObjeto->isChecked() && objetoSelecionado) {
        //qDebug() << "Aplicando transformação no OBJETO.";
        Matriz T = TransformadorGeometrico::translacao(dx, dy, dz);
        objetoSelecionado->aplicarTransformacao(T);
    } else if (ui->rbTransformarCamera->isChecked() && camera) {
        //qDebug() << "Aplicando transformação na CAMERA.";
        camera->transladar(dx, dy, dz);
    }

    ui->frameDesenho->redesenhar();
    resetarControlesTransformacao();
}

void MainWindow::aplicarEscalaAtual() {
    if (!ui->tabWidget->isEnabled()) return;

    double valX = ui->spinEscalaX->value();
    double valY = ui->spinEscalaY->value();
    double valZ = ui->spinEscalaZ->value();
    if (valX == 0 && valY == 0 && valZ == 0) return;

    auto camera = displayFile->obterCameraAtiva();
    if (ui->rbTransformarObjeto->isChecked() && objetoSelecionado) {
        double sx = 1.0 + (valX / 100.0);
        double sy = 1.0 + (valY / 100.0);
        double sz = 1.0 + (valZ / 100.0);
        sx = std::max(0.01, sx);
        sy = std::max(0.01, sy);
        sz = std::max(0.01, sz);

        Ponto3D centroOriginal = objetoSelecionado->calcularCentroGeometrico();
        Ponto3D pivo = objetoSelecionado->obterMatrizTransformacao() * centroOriginal;

        Matriz S = TransformadorGeometrico::escala(sx, sy, sz, pivo);
        objetoSelecionado->aplicarTransformacao(S);

    } else if (ui->rbTransformarCamera->isChecked() && camera) {
        double fatorDolly = valZ * 0.1;
        camera->dolly(fatorDolly);
    }

    ui->frameDesenho->redesenhar();
    resetarControlesTransformacao();
}

void MainWindow::aplicarRotacaoAtual() {
    if (!ui->tabWidget->isEnabled()) return;

    double anguloX = ui->hsRotacaoX->value();
    double anguloY = ui->hsRotacaoY->value();
    double anguloZ = ui->hsRotacaoZ->value();
    if (anguloX == 0 && anguloY == 0 && anguloZ == 0) return;

    auto camera = displayFile->obterCameraAtiva();
    if (ui->rbTransformarObjeto->isChecked() && objetoSelecionado) {
        // 1. Pega o centro do modelo original.
        Ponto3D centroOriginal = objetoSelecionado->calcularCentroGeometrico();
        // 2. Transforma o centro para encontrar a posição atual do pivô no mundo.
        Ponto3D pivo = objetoSelecionado->obterMatrizTransformacao() * centroOriginal;

        Matriz R = TransformadorGeometrico::rotacaoComposta(anguloX, anguloY, anguloZ, pivo);
        objetoSelecionado->aplicarTransformacao(R);

    } else if (ui->rbTransformarCamera->isChecked() && camera) {
        camera->orbitar(anguloY, anguloX);
    }

    ui->frameDesenho->redesenhar();
    resetarControlesTransformacao();
}

// --- OUTROS SLOTS E MÉTODOS ---

void MainWindow::on_btnCor_clicked() {
    if (!objetoSelecionado) {
        QMessageBox::information(this, "Ação Inválida", "Por favor, selecione um objeto para alterar a cor.");
        return;
    }

    QColor cor = QColorDialog::getColor(objetoSelecionado->obterCor(), this, "Selecionar Cor do Objeto");
    if (cor.isValid()) {
        objetoSelecionado->definirCor(cor);
        if (ui->frameDesenho) ui->frameDesenho->redesenhar();
        atualizarCbDisplayFile();
    }
}

void MainWindow::updateTransformationTargetUIState() {
    bool objetoEstaSelecionado = (objetoSelecionado != nullptr);
    bool cameraEstaSelecionada = (displayFile && displayFile->obterCameraAtiva() != nullptr);
    bool alvoEhObjeto = ui->rbTransformarObjeto->isChecked() && objetoEstaSelecionado;
    bool alvoEhCamera = ui->rbTransformarCamera->isChecked() && cameraEstaSelecionada;

    ui->tabWidget->setEnabled(alvoEhObjeto || alvoEhCamera);

    if (objetoEstaSelecionado) {
        corSelecionadaParaDesenho = objetoSelecionado->obterCor();
    }

    resetarControlesTransformacao();
}

void MainWindow::on_btnLimparSelecao_clicked() {
    ui->cbDisplayFile->setCurrentIndex(-1);
    objetoSelecionado = nullptr;
    updateTransformationTargetUIState();
}

void MainWindow::on_btnCriarForma_clicked() {
    GerenciadorObjetosDialog dialog(this->displayFile, this);
    if (dialog.exec() == QDialog::Accepted) {
        auto novoObjeto = dialog.obterObjetoResultante();
        if (novoObjeto) {
            displayFile->adicionarObjeto(novoObjeto);
            ui->frameDesenho->redesenhar();
            atualizarCbDisplayFile();
            int idx = ui->cbDisplayFile->findData(QVariant::fromValue(novoObjeto->obterNome()));
            if(idx != -1) ui->cbDisplayFile->setCurrentIndex(idx);
        }
    }
}

void MainWindow::on_btnModificarForma_clicked()
{
    // A lógica para modificar uma forma pode ser complexa.
    // Por enquanto, uma mensagem resolve o erro de link-edição.
    if (objetoSelecionado) {
        QMessageBox::information(this, "Modificar Forma", "Funcionalidade de modificar ainda não implementada.");
    } else {
        QMessageBox::warning(this, "Modificar Forma", "Nenhum objeto selecionado para modificar.");
    }
}

void MainWindow::on_btnCarregarOBJ_clicked()
{
    QString caminhoArquivo = QFileDialog::getOpenFileName(
        this,
        "Carregar Arquivo .OBJ",
        "",
        "Arquivos de Objeto Wavefront (*.obj)"
        );

    if (caminhoArquivo.isEmpty()) {
        return; // Usuário cancelou a seleção de arquivo
    }

    // --- PASSO 1: OBTER O PONTO DE DESTINO DO USUÁRIO ---
    bool okX, okY, okZ;
    double x = QInputDialog::getDouble(this, "Posição do Objeto", "Digite a coordenada X:", 0, -10000, 10000, 2, &okX);
    if (!okX) return; // Usuário cancelou

    double y = QInputDialog::getDouble(this, "Posição do Objeto", "Digite a coordenada Y:", 0, -10000, 10000, 2, &okY);
    if (!okY) return; // Usuário cancelou

    double z = QInputDialog::getDouble(this, "Posição do Objeto", "Digite a coordenada Z:", 0, -10000, 10000, 2, &okZ);
    if (!okZ) return; // Usuário cancelou

    Ponto3D pontoDestino(x, y, z);
    // --- FIM DO PASSO 1 ---

    auto novoObjeto = CarregadorOBJ::carregar(caminhoArquivo);

    if (novoObjeto) {
        // --- PASSO 2: CALCULAR E APLICAR A TRANSLAÇÃO ---
        BoundingBox bboxInicial = novoObjeto->obterBBox();
        if (bboxInicial.ehValida()) {
            Ponto3D centroOriginal = bboxInicial.obterCentro();
            Ponto3D vetorTranslacao = pontoDestino - centroOriginal;
            Matriz matTranslacao = Matriz::translacao(
                vetorTranslacao.obterX(),
                vetorTranslacao.obterY(),
                vetorTranslacao.obterZ()
                );
            novoObjeto->aplicarTransformacao(matTranslacao);
        }
        // --- FIM DO PASSO 2 ---

        // 1. Adiciona o objeto ao modelo de dados.
        displayFile->adicionarObjeto(novoObjeto);

        // 2. Atualiza o ComboBox para listar o novo objeto.
        atualizarCbDisplayFile();

        // 3. Define o estado manualmente, sem depender de sinais.
        //    Isso garante que o ponteiro e o alvo da transformação estejam corretos.
        objetoSelecionado = novoObjeto;
        ui->rbTransformarObjeto->setChecked(true);

        // 4. Apenas atualiza a APARÊNCIA do ComboBox, bloqueando os sinais
        //    para garantir que a função de seleção não seja chamada desnecessariamente.
        int idx = ui->cbDisplayFile->findData(QVariant::fromValue(novoObjeto->obterNome()));
        if (idx != -1) {
            ui->cbDisplayFile->blockSignals(true);
            ui->cbDisplayFile->setCurrentIndex(idx);
            ui->cbDisplayFile->blockSignals(false);
        }

        // 5. Atualiza o estado geral da UI e executa ações secundárias.
        updateTransformationTargetUIState();
        focarNoObjeto(novoObjeto);
        ui->frameDesenho->redesenhar();

        QMessageBox::information(this, "Sucesso", "Objeto '" + novoObjeto->obterNome() + "' carregado na posição (" + QString::number(x) + ", " + QString::number(y) + ", " + QString::number(z) + ").");
    } else {
        QMessageBox::warning(this, "Erro de Carregamento", "Não foi possível carregar o arquivo .obj selecionado.");
    }
}

void MainWindow::on_btnExcluirForma_clicked()
{
    // 1. Verifica se há um objeto selecionado para excluir.
    if (!objetoSelecionado) {
        QMessageBox::warning(this, "Excluir Forma", "Nenhum objeto selecionado para excluir.");
        return;
    }

    // 2. Pede confirmação do usuário antes de realizar uma ação destrutiva.
    QMessageBox::StandardButton resposta = QMessageBox::question(
        this,
        "Confirmar Exclusão",
        "Você tem certeza que deseja excluir o objeto '" + objetoSelecionado->obterNome() + "'?",
        QMessageBox::Yes | QMessageBox::No
        );

    // 3. Se o usuário confirmar, procede com a exclusão.
    if (resposta == QMessageBox::Yes) {
        QString nomeObjetoParaExcluir = objetoSelecionado->obterNome();

        // Limpa a seleção atual para evitar um ponteiro inválido
        objetoSelecionado = nullptr;

        // Remove o objeto do display file
        // (Assumindo que você tem um método 'removerObjeto' na sua classe DisplayFile)
        displayFile->removerObjeto(nomeObjetoParaExcluir);

        // Atualiza a interface gráfica
        atualizarCbDisplayFile();
        ui->frameDesenho->redesenhar();
    }
}

void MainWindow::focarNoObjeto(std::shared_ptr<ObjetoGrafico> objeto) {
    if (!objeto || !displayFile || !displayFile->obterCameraAtiva()) {
        return;
    }

    auto camera = displayFile->obterCameraAtiva();

    BoundingBox bbox = objeto->obterBBox();
    if (!bbox.ehValida()) return;

    Ponto3D centroObjeto = bbox.obterCentro();

    Ponto3D pMin = bbox.obterMin();
    Ponto3D pMax = bbox.obterMax();

    Ponto3D vetorDiagonal = pMax - pMin;
    double tamanho = vetorDiagonal.magnitude();

    if (tamanho < 1.0) tamanho = 1.0;

    // Define o novo alvo da câmera
    camera->definirAlvo(centroObjeto);

    // Calcula a nova posição da câmera
    Ponto3D direcaoVisao = (camera->obterAlvo() - camera->obterPosicao()).normalizarVetor();
    Ponto3D novaPosicaoCamera = centroObjeto - (direcaoVisao * tamanho * 1.5);

    camera->definirPosicao(novaPosicaoCamera);
}
