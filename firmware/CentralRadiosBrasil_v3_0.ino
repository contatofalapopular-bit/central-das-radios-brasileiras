/*
  PROJETO: CENTRAL RÁDIOS BRASIL
  VERSÃO: 1.9.1 - OTA de produção pelo GitHub
  

  A logo está gravada dentro do firmware.
  Não usa hospedagem, domínio ou mensalidade.
*/

#include <VS1053.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>


// PINOS DO VS1053
#define VS1053_CS    5
#define VS1053_DCS   16
#define VS1053_DREQ  4

// BOTÕES
#define BOTAO_ANTERIOR 21
#define BOTAO_PROXIMA  22

#define VOLUME 80
#define MAX_RADIOS 500

const char *URL_LISTA_RADIOS =
"https://raw.githubusercontent.com/contatofalapopular-bit/central-radios-brasil/main/radios.json";

#define VERSAO_FIRMWARE "3.0.1"
#define CODIGO_VERSAO_FIRMWARE 1092

const char *URL_MANIFESTO_OTA =
"https://raw.githubusercontent.com/contatofalapopular-bit/central-radios-brasil/main/firmware.json";

const unsigned long INTERVALO_VERIFICACAO_OTA = 21600000UL; // 6 horas
const unsigned long TEMPO_LIMITE_OTA = 30000UL;             // 30 segundos
const size_t TAMANHO_MAXIMO_MANIFESTO = 4096;
const uint32_t HEAP_MINIMO_PARA_OTA = 100000;

const unsigned long INTERVALO_RECONEXAO_WIFI = 10000UL;
const unsigned long INTERVALO_RECONEXAO_RADIO = 5000UL;
const unsigned long INTERVALO_NOVA_TENTATIVA_GITHUB = 300000UL; // 5 minutos
const unsigned long TEMPO_MAXIMO_SEM_DADOS_AUDIO = 20000UL;     // 20 segundos

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);
WiFiClient client;
WiFiClientSecure secureClient;
WiFiClientSecure otaClient;
Preferences memoria;
const int MAX_FAVORITAS = 5;

int favoritas[MAX_FAVORITAS];
int quantidadeFavoritas = 0;
bool assistenteInicialConcluido = false;

Client *radioClient = &client;
WebServer servidor(80);
DNSServer servidorDNS;
struct Radio {
  String id;
  String nome;
  String cidade;
  String estado;
  String categoria;
  String host;
  String caminho;
  uint16_t porta;
  bool ssl;
};

Radio radios[MAX_RADIOS];
int totalRadios = 0;
int estacao = 0;
uint8_t volumeAtual = VOLUME;
uint8_t mp3buff[128];

bool usandoListaReserva = false;
unsigned long ultimaTentativaWiFi = 0;
unsigned long ultimaTentativaRadio = 0;
unsigned long ultimaTentativaGitHub = 0;
unsigned long ultimoDadoAudio = 0;
unsigned long inicioBoot = 0;
unsigned long inicioConexaoAtual = 0;
unsigned long ultimaVerificacaoOTA = 0;

bool otaEmAndamento = false;

unsigned long totalReconexoesWiFi = 0;
unsigned long totalReconexoesStream = 0;
unsigned long totalFalhasWiFi = 0;
unsigned long totalFalhasStream = 0;
unsigned long totalDownloadsGitHub = 0;
unsigned long totalFalhasGitHub = 0;
unsigned long totalVerificacoesOTA = 0;
unsigned long totalFalhasOTA = 0;

void conectaWiFi();
bool baixarListaRadios();
void carregarRadioReserva();
void conectaRadio();
void nextEstacao();
void prevEstacao();
void carregarMemoria();
void salvarFavoritas();
void salvarEstacao();
void salvarVolume();
void verificarAtualizacaoLista();
void mostrarEtapa(uint8_t etapa, const char *mensagem);
void mostrarDiagnostico();
void mostrarInformacoesRadio();
void verificarOTA(bool forcarVerificacao);
bool versaoOTAMaisNova(int codigoRemoto);
void prepararParaOTA();
void verificarResultadoOTAAnterior();
String gerarPaginaCentral();
String gerarPaginaWiFi();
String gerarPaginaInfo();
void iniciarPainelCentral();





// HTML/CSS PROFISSIONAL DO PORTAL
String criarCabecalhoPortal() {
  String html;
  html.reserve(22000);

  html += F("<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1'>");
  html += F("<style>");

  // FUNDO GERAL
  html += F("*{box-sizing:border-box;}");
  html += F("html{min-height:100%;background:#020b22;}");
  html += F("body{min-height:100vh;margin:0!important;padding:20px 14px 34px!important;");
  html += F("background:radial-gradient(circle at top,#123b91 0,#061a4b 34%,#020b22 74%);");
  html += F("color:#f7f9ff!important;font-family:Arial,Helvetica,sans-serif!important;}");

  // LOGO EMBUTIDA


  // CARTÃO PRINCIPAL
  html += F(".wrap{width:100%!important;max-width:440px!important;margin:0 auto!important;");
  html += F("padding:22px 18px 20px!important;background:rgba(255,255,255,.97)!important;");
  html += F("border:1px solid rgba(255,255,255,.45)!important;border-radius:22px!important;");
  html += F("box-shadow:0 18px 46px rgba(0,0,0,.38)!important;color:#07163d!important;}");

  // TÍTULO E TEXTO DE ORIENTAÇÃO
  html += F("h1{font-size:0!important;line-height:1.15!important;margin:0 0 20px!important;");
  html += F("text-align:center!important;color:#07163d!important;}");
  html += F("h1:before{content:'CENTRAL RÁDIOS BRASIL';display:block;");
  html += F("font-size:23px;line-height:1.16;font-weight:800;letter-spacing:.2px;");
  html += F("color:#07163d;margin-bottom:9px;}");
  html += F("h1:after{content:'Configure o Wi-Fi para começar a ouvir suas rádios.';display:block;");
  html += F("font-size:14px;line-height:1.45;font-weight:400;color:#53617f;");
  html += F("max-width:330px;margin:0 auto;}");

  // TEXTOS
  html += F("h2,h3{color:#07163d!important;}");
  html += F("p,div,span,label{font-family:Arial,Helvetica,sans-serif;}");
  html += F("label{color:#1b2b51!important;font-size:14px!important;font-weight:700!important;}");
  html += F(".msg{border-radius:12px!important;padding:12px!important;}");

  // CAMPOS
  html += F("input,select{width:100%!important;min-height:48px!important;");
  html += F("border:1px solid #c8d1e3!important;border-radius:12px!important;");
  html += F("background:#f8faff!important;color:#07163d!important;font-size:16px!important;");
  html += F("padding:11px 13px!important;outline:none!important;}");
  html += F("input:focus,select:focus{border-color:#174fc5!important;");
  html += F("box-shadow:0 0 0 3px rgba(23,79,197,.15)!important;}");

  // BOTÕES
  html += F("button,.btn,input[type=submit],input[type=button]{width:100%!important;");
  html += F("min-height:50px!important;border:0!important;border-radius:13px!important;");
  html += F("background:linear-gradient(180deg,#ffe13e,#f6bf00)!important;");
  html += F("color:#061331!important;font-size:16px!important;font-weight:800!important;");
  html += F("letter-spacing:.2px!important;box-shadow:0 8px 18px rgba(223,164,0,.28)!important;");
  html += F("cursor:pointer!important;}");
  html += F("button:active,.btn:active,input[type=submit]:active{transform:translateY(1px);}");

  // LINKS E LISTA DE REDES
  html += F("a{color:#174fc5!important;font-weight:700!important;text-decoration:none!important;}");
  html += F(".q{border-radius:12px!important;}");
  html += F(".wifi,.scan{border-radius:12px!important;}");
  html += F("hr{border:0!important;border-top:1px solid #e1e6f0!important;margin:18px 0!important;}");

  // AVISO DE SEGURANÇA / RODAPÉ
  html += F(".wrap:after{content:'Conexão segura • A senha fica salva somente neste aparelho';");
  html += F("display:block;margin-top:18px;padding-top:15px;border-top:1px solid #e3e8f2;");
  html += F("text-align:center;color:#64708a;font-size:11px;line-height:1.4;}");
  html += F("body:after{content:'CENTRAL RÁDIOS BRASIL';display:block;");
  html += F("max-width:440px;margin:14px auto 0;text-align:center;color:#b8c8f6;");
  html += F("font-size:11px;font-weight:700;letter-spacing:1.2px;}");

  // AJUSTES PARA TELAS PEQUENAS
  html += F("@media(max-width:360px){body{padding-left:9px!important;padding-right:9px!important;}");
  html += F("body:before{width:150px;height:150px;}.wrap{padding:18px 14px!important;}");
  html += F("h1:before{font-size:20px;}}");

  html += F("</style>");

  return html;
}

/*
  As rádios não ficam mais fixas aqui.
  O ESP32 baixa o arquivo radios.json do GitHub.

  Caso o GitHub esteja indisponível, a Rádio Fala Popular
  é carregada como estação de reserva.
*/

 void setup() {
  inicioBoot = millis();
  Serial.begin(115200);

  pinMode(BOTAO_ANTERIOR, INPUT_PULLUP);
  pinMode(BOTAO_PROXIMA, INPUT_PULLUP);

  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("CENTRAL RADIOS BRASIL");
  Serial.print("Firmware ");
  Serial.println(VERSAO_FIRMWARE);
  Serial.println("Atualizacao remota OTA pelo GitHub");
  Serial.println("========================================");

  mostrarEtapa(1, "Inicializando ESP32");

  mostrarEtapa(2, "Carregando memoria");
  memoria.begin("centralradios", false);
  verificarResultadoOTAAnterior();
  carregarMemoria();

  mostrarEtapa(3, "Inicializando VS1053");
  SPI.begin();
  player.begin();

  if (player.getChipVersion() == 4) {
    player.loadDefaultVs1053Patches();
  }

  player.switchToMp3Mode();
  player.setVolume(volumeAtual);

  mostrarEtapa(4, "Conectando ao Wi-Fi");
  conectaWiFi();

  mostrarEtapa(5, "Verificando atualizacao OTA");
  verificarOTA(true);

  mostrarEtapa(6, "Baixando lista do GitHub");
  ultimaTentativaGitHub = millis();

  if (!baixarListaRadios()) {
    carregarRadioReserva();
  } else {
    usandoListaReserva = false;
  }

mostrarEtapa(7, "Carregando radios");
Serial.print("Radios disponiveis: ");
Serial.println(totalRadios);

organizarRadiosComFavoritas();

mostrarEtapa(8, "Conectando a emissora");

if (!assistenteInicialConcluido && totalRadios >= MAX_FAVORITAS) {

  Serial.println();
  Serial.println("Primeira configuracao.");
  Serial.println("Escolha as 5 favoritas pelo celular.");

  iniciarServidorFavoritas();

  Serial.print("Abra no celular: http://");
  Serial.println(WiFi.localIP());

}

conectaRadio();
iniciarPainelCentral();
  mostrarEtapa(9, "Sistema pronto");
  mostrarDiagnostico();
}
void loop() {
  unsigned long agora = millis();

if (!assistenteInicialConcluido) {
  servidorDNS.processNextRequest();
}

servidor.handleClient();

  bool anteriorPressionado =
    digitalRead(BOTAO_ANTERIOR) == LOW;

  bool proximaPressionada =
    digitalRead(BOTAO_PROXIMA) == LOW;

  // Troca de rádio somente quando um único botão é pressionado.
  if (anteriorPressionado && !proximaPressionada) {
    prevEstacao();

    while (
      digitalRead(BOTAO_ANTERIOR) == LOW &&
      digitalRead(BOTAO_PROXIMA) == HIGH
    ) {
      delay(10);
    }
  }

  if (proximaPressionada && !anteriorPressionado) {
    nextEstacao();

    while (
      digitalRead(BOTAO_PROXIMA) == LOW &&
      digitalRead(BOTAO_ANTERIOR) == HIGH
    ) {
      delay(10);
    }
  }

  // Reconexão do Wi-Fi sem travar o programa continuamente.
  if (WiFi.status() != WL_CONNECTED) {
    if (agora - ultimaTentativaWiFi >= INTERVALO_RECONEXAO_WIFI) {
      ultimaTentativaWiFi = agora;

      Serial.println();
      Serial.println("Wi-Fi desconectado. Tentando reconectar...");
      totalReconexoesWiFi++;
      WiFi.reconnect();
    }

    delay(10);
    return;
  }

  // Verifica atualização OTA periodicamente.
  verificarOTA(false);

  // Se iniciou com a lista de reserva, tenta novamente o GitHub a cada 5 minutos.
  verificarAtualizacaoLista();

  // Recebe e envia áudio para o VS1053.
  if (radioClient->available() > 0) {
    int bytesread = radioClient->read(mp3buff, 64);

    if (bytesread > 0) {
      ultimoDadoAudio = agora;
      player.playChunk(mp3buff, bytesread);
    }
  }

  bool semDadosHaMuitoTempo =
    ultimoDadoAudio > 0 &&
    (agora - ultimoDadoAudio >= TEMPO_MAXIMO_SEM_DADOS_AUDIO);

  // Reconecta se o socket fechou ou se o stream ficou sem enviar áudio.
  if (totalRadios > 0 &&
      (!radioClient->connected() || semDadosHaMuitoTempo) &&
      agora - ultimaTentativaRadio >= INTERVALO_RECONEXAO_RADIO) {

    ultimaTentativaRadio = agora;

    totalReconexoesStream++;

    if (semDadosHaMuitoTempo) {
      Serial.println("Stream ficou sem dados de audio. Reconectando...");
    } else {
      Serial.println("Stream desconectado. Reconectando...");
    }

    conectaRadio();
  }

  delay(1);
}

void conectaWiFi() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;

  wm.setHostname("central-radios-brasil");
  wm.setConfigPortalTimeout(300);

  // Personaliza a página com a logo gravada no firmware.
  String cabecalhoPortal = criarCabecalhoPortal();
  wm.setCustomHeadElement(cabecalhoPortal.c_str());

  // Título exibido pelo WiFiManager.
  wm.setTitle("CENTRAL RÁDIOS BRASIL");

  Serial.println();
  Serial.println("========================================");
  Serial.println("CENTRAL RADIOS BRASIL");
  Serial.println("Conectando ao Wi-Fi...");
  Serial.println("========================================");

  // Rede aberta criada apenas quando não há Wi-Fi salvo.
 bool conectado = wm.autoConnect("CENTRAL_RADIOS_BRASIL");

  if (!conectado) {
    totalFalhasWiFi++;
    Serial.println("Nao foi possivel conectar ao Wi-Fi.");
    Serial.println("Reiniciando o aparelho...");
    delay(3000);
    ESP.restart();
  }

  Serial.println();
  Serial.println("Wi-Fi conectado com sucesso!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

bool baixarListaRadios() {
  ultimaTentativaGitHub = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem Wi-Fi para baixar a lista.");
    return false;
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("BAIXANDO LISTA DE RADIOS DO GITHUB");
  Serial.println("========================================");

  WiFiClientSecure githubClient;
  githubClient.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);

  if (!http.begin(githubClient, URL_LISTA_RADIOS)) {
    Serial.println("Não foi possível iniciar o acesso ao GitHub.");
    return false;
  }

  http.addHeader("User-Agent", "CentralDasRadios-ESP32/1.9.1");

  int codigoHttp = http.GET();
  totalDownloadsGitHub++;

  if (codigoHttp != HTTP_CODE_OK) {
    totalFalhasGitHub++;
    Serial.print("Falha ao baixar radios.json. Código HTTP: ");
    Serial.println(codigoHttp);
    http.end();
    return false;
  }

  int tamanhoManifesto = http.getSize();

  if (tamanhoManifesto > (int)TAMANHO_MAXIMO_MANIFESTO) {
    totalFalhasOTA++;
    Serial.println("Manifesto OTA maior que o limite permitido.");
    http.end();
   return false;
  }

  String conteudo = http.getString();
  http.end();

  if (conteudo.length() == 0 ||
      conteudo.length() > TAMANHO_MAXIMO_MANIFESTO) {
    totalFalhasOTA++;
    Serial.println("Manifesto OTA vazio ou invalido.");
   return false;
  }

  JsonDocument documento;
  DeserializationError erro = deserializeJson(documento, conteudo);

  if (erro) {
    totalFalhasGitHub++;
    Serial.print("Erro ao interpretar radios.json: ");
    Serial.println(erro.c_str());
    return false;
  }

  JsonArray lista = documento["radios"].as<JsonArray>();

  if (lista.isNull()) {
    totalFalhasGitHub++;
    Serial.println("O arquivo nao contem a lista 'radios'.");
    return false;
  }

  totalRadios = 0;

  for (JsonObject item : lista) {
    if (totalRadios >= MAX_RADIOS) {
      Serial.println("Limite máximo de rádios atingido.");
      break;
    }

    bool ativa = item["ativa"] | true;

    if (!ativa) {
      continue;
    }

    const char *nome = item["nome"] | "";
    const char *host = item["host"] | "";
    const char *caminho = item["caminho"] | "/";
    int porta = item["porta"] | 80;

    if (strlen(nome) == 0 || strlen(host) == 0 || porta <= 0) {
      Serial.println("Rádio ignorada por dados incompletos.");
      continue;
    }

    radios[totalRadios].id = item["id"] | "";
    radios[totalRadios].nome = nome;
    radios[totalRadios].cidade = item["cidade"] | "";
    radios[totalRadios].estado = item["estado"] | "";
    radios[totalRadios].categoria = item["categoria"] | "";
    radios[totalRadios].host = host;
    radios[totalRadios].porta = (uint16_t)porta;
    radios[totalRadios].caminho = caminho;
    radios[totalRadios].ssl = item["ssl"] | false;

    totalRadios++;
  }

  if (totalRadios == 0) {
    totalFalhasGitHub++;
    Serial.println("Nenhuma radio ativa foi encontrada.");
    return false;
  }

  if (estacao < 0 || estacao >= totalRadios) {
    estacao = 0;
    salvarEstacao();
  }

  usandoListaReserva = false;

  Serial.print("Lista carregada com sucesso. Total de rádios: ");
  Serial.println(totalRadios);

  for (int i = 0; i < totalRadios; i++) {
    Serial.print(i + 1);
    Serial.print(" - ");
    Serial.println(radios[i].nome);
  }

  return true;
}

void carregarRadioReserva() {
  usandoListaReserva = true;

  Serial.println();
  Serial.println("Usando a Rádio Fala Popular como reserva.");

  totalRadios = 1;
  estacao = 0;
  salvarEstacao();

  radios[0].id = "fala-popular";
  radios[0].nome = "Rádio Fala Popular";
  radios[0].cidade = "Rio Verde";
  radios[0].estado = "GO";
  radios[0].categoria = "Jornalismo e Variedades";
  radios[0].host = "stm19.xcast.com.br";
  radios[0].porta = 10792;
  radios[0].caminho = "/";
  radios[0].ssl = false;
}

void conectaRadio() {
  ultimoDadoAudio = millis();

  if (totalRadios <= 0) {
    Serial.println("Não há rádios disponíveis.");
    return;
  }

  radioClient->stop();

  if (radios[estacao].ssl) {
    secureClient.setInsecure();
    radioClient = &secureClient;
  } else {
    radioClient = &client;
  }

  radioClient->stop();

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.print("Estação: ");
  Serial.print(estacao + 1);
  Serial.print(" de ");
  Serial.println(totalRadios);

  Serial.print("Nome: ");
  Serial.println(radios[estacao].nome);

  Serial.print("Cidade/UF: ");
  Serial.print(radios[estacao].cidade);
  Serial.print("/");
  Serial.println(radios[estacao].estado);

  Serial.print("Servidor: ");
  Serial.println(radios[estacao].host);

  Serial.print("Porta: ");
  Serial.println(radios[estacao].porta);

  Serial.print("SSL: ");
  Serial.println(radios[estacao].ssl ? "sim" : "não");
  Serial.println("----------------------------------------");

  if (!radioClient->connect(
        radios[estacao].host.c_str(),
        radios[estacao].porta)) {
    totalFalhasStream++;
    Serial.println("Falha ao conectar a estacao.");
    ultimaTentativaRadio = millis();
    return;
  }

  radioClient->print(
    String("GET ") + radios[estacao].caminho + " HTTP/1.1\r\n" +
    "Host: " + radios[estacao].host + "\r\n" +
    "User-Agent: CentralDasRadios/1.9.1\r\n" +
    "Accept: */*\r\n" +
    "Icy-MetaData: 0\r\n" +
    "Connection: close\r\n\r\n"
  );

  inicioConexaoAtual = millis();
  Serial.println("Conectado a estacao.");
  mostrarInformacoesRadio();
}

void nextEstacao() {
  if (totalRadios <= 0) {
    return;
  }

  estacao++;

  if (estacao < 0 || estacao >= totalRadios) {
    estacao = 0;
  }

  salvarEstacao();

  Serial.print("Mudando para: ");
  Serial.println(radios[estacao].nome);

  conectaRadio();
  delay(500);
}

void prevEstacao() {
  if (totalRadios <= 0) {
    return;
  }

  estacao--;

  if (estacao < 0) {
    estacao = totalRadios - 1;
  }

  salvarEstacao();

  Serial.print("Mudando para: ");
  Serial.println(radios[estacao].nome);

  conectaRadio();
  delay(500);
}



void verificarAtualizacaoLista() {
  if (!usandoListaReserva) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  unsigned long agora = millis();

  if (agora - ultimaTentativaGitHub < INTERVALO_NOVA_TENTATIVA_GITHUB) {
    return;
  }

  Serial.println();
  Serial.println("Nova tentativa de baixar a lista oficial do GitHub...");

  if (baixarListaRadios()) {
    Serial.println("Lista oficial recuperada. Saindo do modo de reserva.");

    if (estacao < 0 || estacao >= totalRadios) {
      estacao = 0;
      salvarEstacao();
    }

    conectaRadio();
  } else {
    Serial.println("GitHub ainda indisponível. Mantendo a rádio de reserva.");
  }
}

void carregarMemoria() {
  estacao = memoria.getInt("estacao", 0);
  volumeAtual = memoria.getUChar("volume", VOLUME);
    quantidadeFavoritas = memoria.getInt("qtdFav", 0);
assistenteInicialConcluido = memoria.getBool("assistenteOK", false);
    for (int i = 0; i < MAX_FAVORITAS; i++) {
        favoritas[i] = memoria.getInt(
            ("fav" + String(i)).c_str(),
            -1
        );
    }
  if (volumeAtual > 100) {
    volumeAtual = VOLUME;
    salvarVolume();
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("MEMORIA DO APARELHO");
  Serial.println("========================================");
  Serial.print("Ultima estacao salva: ");
  Serial.println(estacao + 1);
  Serial.print("Volume salvo: ");
  Serial.println(volumeAtual);
}

void salvarEstacao() {
  memoria.putInt("estacao", estacao);
  Serial.print("Estacao salva na memoria: ");
  Serial.println(estacao + 1);
}
void salvarFavoritas() {

    memoria.putInt("qtdFav", quantidadeFavoritas);

    for (int i = 0; i < MAX_FAVORITAS; i++) {
        memoria.putInt(
            ("fav" + String(i)).c_str(),
            favoritas[i]
        );
    }

}
bool radioEhFavorita(int indiceRadio) {

  for (int i = 0; i < quantidadeFavoritas; i++) {

    if (favoritas[i] == indiceRadio) {
      return true;
    }
  }

  return false;
}

bool alternarFavoritaAtual() {

  if (
    totalRadios <= 0 ||
    estacao < 0 ||
    estacao >= totalRadios
  ) {
    return false;
  }

  // Se já for favorita, remove.
  for (int i = 0; i < quantidadeFavoritas; i++) {

    if (favoritas[i] == estacao) {

      for (int j = i; j < quantidadeFavoritas - 1; j++) {
        favoritas[j] = favoritas[j + 1];
      }

      quantidadeFavoritas--;

      if (quantidadeFavoritas >= 0 &&
          quantidadeFavoritas < MAX_FAVORITAS) {
        favoritas[quantidadeFavoritas] = -1;
      }

      salvarFavoritas();
      return false;
    }
  }

  // Se ainda não for favorita, adiciona.
  if (quantidadeFavoritas >= MAX_FAVORITAS) {
    return false;
  }

  favoritas[quantidadeFavoritas] = estacao;
  quantidadeFavoritas++;

  salvarFavoritas();
  return true;
}
void salvarVolume() {
  memoria.putUChar("volume", volumeAtual);
  Serial.print("Volume salvo na memoria: ");
  Serial.println(volumeAtual);
}




void verificarResultadoOTAAnterior() {
  int codigoPendente = memoria.getInt("ota_codigo", 0);
  String versaoPendente = memoria.getString("ota_versao", "");

  if (codigoPendente == 0) {
    return;
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("RESULTADO DA ULTIMA OTA");
  Serial.println("========================================");
  Serial.print("Versao esperada....: ");
  Serial.println(versaoPendente);
  Serial.print("Codigo esperado....: ");
  Serial.println(codigoPendente);
  Serial.print("Versao instalada...: ");
  Serial.println(VERSAO_FIRMWARE);
  Serial.print("Codigo instalado...: ");
  Serial.println(CODIGO_VERSAO_FIRMWARE);

  if (codigoPendente == CODIGO_VERSAO_FIRMWARE) {
    Serial.println("Resultado...........: ATUALIZACAO CONFIRMADA");
  } else {
    Serial.println("Resultado...........: VERSAO NAO CONFIRMADA");
  }

  memoria.remove("ota_codigo");
  memoria.remove("ota_versao");
}

bool versaoOTAMaisNova(int codigoRemoto) {
  return codigoRemoto > CODIGO_VERSAO_FIRMWARE;
}

void prepararParaOTA() {
  otaEmAndamento = true;

  Serial.println();
  Serial.println("========================================");
  Serial.println("PREPARANDO ATUALIZACAO OTA");
  Serial.println("========================================");
  Serial.println("Interrompendo o stream de audio...");

 Serial.println("Stream mantido durante o teste OTA.");

  delay(300);
}

void verificarOTA(bool forcarVerificacao) {
  if (otaEmAndamento || WiFi.status() != WL_CONNECTED) {
    return;
  }

  unsigned long agora = millis();

  if (!forcarVerificacao &&
      (agora - ultimaVerificacaoOTA < INTERVALO_VERIFICACAO_OTA)) {
    return;
  }

  ultimaVerificacaoOTA = agora;
  totalVerificacoesOTA++;

  Serial.println();
  Serial.println("========================================");
  Serial.println("VERIFICACAO DE ATUALIZACAO OTA");
  Serial.println("========================================");
  Serial.print("Versao instalada..: ");
  Serial.println(VERSAO_FIRMWARE);
  Serial.println("Consultando firmware.json...");

  WiFiClientSecure manifestoClient;
  manifestoClient.setInsecure();

  HTTPClient http;
  http.setTimeout(TEMPO_LIMITE_OTA);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(manifestoClient, URL_MANIFESTO_OTA)) {
    totalFalhasOTA++;
    Serial.println("Nao foi possivel iniciar a consulta OTA.");
    return;
  }

  http.addHeader("User-Agent", "CentralDasRadios-ESP32/1.9.1");
  int codigoHttp = http.GET();

  if (codigoHttp != HTTP_CODE_OK) {
    totalFalhasOTA++;
    Serial.print("Manifesto OTA indisponivel. HTTP: ");
    Serial.println(codigoHttp);
    http.end();
    return;
  }

  String conteudo = http.getString();
  http.end();

  JsonDocument documento;
  DeserializationError erro = deserializeJson(documento, conteudo);

  if (erro) {
    totalFalhasOTA++;
    Serial.print("Erro ao interpretar firmware.json: ");
    Serial.println(erro.c_str());
    return;
  }

  if (!documento["ativa"].is<bool>() ||
      !documento["versao"].is<const char *>() ||
      !documento["versao_codigo"].is<int>() ||
      !documento["bin_url"].is<const char *>()) {
    totalFalhasOTA++;
    Serial.println("Manifesto OTA incompleto ou com campos invalidos.");
    return;
  }

  bool ativa = documento["ativa"].as<bool>();
  String versaoRemota = documento["versao"].as<String>();
  int codigoRemoto = documento["versao_codigo"].as<int>();
  String urlBinario = documento["bin_url"].as<String>();

  Serial.print("Versao disponivel..: ");
  Serial.println(versaoRemota);
  Serial.print("Codigo remoto......: ");
  Serial.println(codigoRemoto);

  if (!ativa) {
    Serial.println("Atualizacoes OTA estao desativadas no manifesto.");
    return;
  }

  if (!versaoOTAMaisNova(codigoRemoto)) {
    Serial.println("O firmware ja esta atualizado.");
    return;
  }

  if (!urlBinario.startsWith("https://") ||
      !urlBinario.endsWith(".bin")) {
    totalFalhasOTA++;
    Serial.println("A URL do firmware deve usar HTTPS e terminar em .bin.");
    return;
  }

  if (ESP.getFreeHeap() < HEAP_MINIMO_PARA_OTA) {
    totalFalhasOTA++;
    Serial.print("Heap insuficiente para iniciar OTA: ");
    Serial.println(ESP.getFreeHeap());
    return;
  }

  Serial.println();
  Serial.println("NOVA VERSAO ENCONTRADA!");
  Serial.print("Atualizando de ");
  Serial.print(VERSAO_FIRMWARE);
  Serial.print(" para ");
  Serial.println(versaoRemota);
  Serial.println("Nao desligue o aparelho.");

  memoria.putInt("ota_codigo", codigoRemoto);
  memoria.putString("ota_versao", versaoRemota);

  prepararParaOTA();

  otaClient.setInsecure();
  httpUpdate.rebootOnUpdate(true);
Serial.println("INICIANDO DOWNLOAD DO BIN...");
Serial.println(urlBinario);

  t_httpUpdate_return resultado = httpUpdate.update(otaClient, urlBinario);
Serial.println("RETORNOU DO HTTPUPDATE");
  switch (resultado) {
    case HTTP_UPDATE_FAILED:
      otaEmAndamento = false;
      memoria.remove("ota_codigo");
      memoria.remove("ota_versao");
      totalFalhasOTA++;
      Serial.printf(
        "Falha OTA (%d): %s\n",
        httpUpdate.getLastError(),
        httpUpdate.getLastErrorString().c_str()
      );
      break;

    case HTTP_UPDATE_NO_UPDATES:
      otaEmAndamento = false;
      memoria.remove("ota_codigo");
      memoria.remove("ota_versao");
      Serial.println("O servidor informou que nao ha atualizacao.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("Atualizacao concluida. Reiniciando...");
      break;
  }
}

void mostrarEtapa(uint8_t etapa, const char *mensagem) {
  Serial.print("[");
  Serial.print(etapa);
  Serial.print("/9] ");
  Serial.println(mensagem);
}

void mostrarInformacoesRadio() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("RADIO CONECTADA");
  Serial.println("========================================");
  Serial.print("Radio.............: ");
  Serial.println(radios[estacao].nome);
  Serial.print("Cidade/UF.........: ");
  Serial.print(radios[estacao].cidade);
  Serial.print("/");
  Serial.println(radios[estacao].estado);
  Serial.print("Categoria.........: ");
  Serial.println(radios[estacao].categoria);
  Serial.print("Host..............: ");
  Serial.println(radios[estacao].host);
  Serial.print("Porta.............: ");
  Serial.println(radios[estacao].porta);
  Serial.print("Caminho...........: ");
  Serial.println(radios[estacao].caminho);
  Serial.print("SSL...............: ");
  Serial.println(radios[estacao].ssl ? "sim" : "nao");
  Serial.print("Status............: ");
  Serial.println(radioClient->connected() ? "Conectado" : "Desconectado");
}

void mostrarDiagnostico() {
  unsigned long tempoBoot = millis() - inicioBoot;

  Serial.println();
  Serial.println("========================================");
  Serial.println("DIAGNOSTICO DO SISTEMA");
  Serial.println("========================================");
  Serial.print("Firmware..........: ");
  Serial.print(VERSAO_FIRMWARE);
  Serial.println();
  Serial.print("Tempo de boot.....: ");
  Serial.print(tempoBoot / 1000.0, 2);
  Serial.println(" s");
  Serial.print("Heap livre........: ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");
  Serial.print("Wi-Fi.............: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Conectado" : "Desconectado");
  Serial.print("Endereco IP.......: ");
  Serial.println(WiFi.localIP());
  Serial.print("Sinal Wi-Fi.......: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("Lista de radios...: ");
  Serial.println(usandoListaReserva ? "Reserva" : "GitHub");
  Serial.print("Total de radios...: ");
  Serial.println(totalRadios);
  Serial.print("Estacao atual.....: ");
  Serial.println(estacao + 1);
  Serial.print("Volume............: ");
  Serial.println(volumeAtual);
  Serial.print("Downloads GitHub..: ");
  Serial.println(totalDownloadsGitHub);
  Serial.print("Falhas GitHub.....: ");
  Serial.println(totalFalhasGitHub);
  Serial.print("Verificacoes OTA..: ");
  Serial.println(totalVerificacoesOTA);
  Serial.print("Falhas OTA........: ");
  Serial.println(totalFalhasOTA);
  Serial.print("Reconexoes Wi-Fi..: ");
  Serial.println(totalReconexoesWiFi);
  Serial.print("Falhas Wi-Fi......: ");
  Serial.println(totalFalhasWiFi);
  Serial.print("Reconexoes stream.: ");
  Serial.println(totalReconexoesStream);
  Serial.print("Falhas stream.....: ");
  Serial.println(totalFalhasStream);
  Serial.println("========================================");
}
//======================================================
// CENTRAL RÁDIOS BRASIL V3.0
// FAVORITAS
//======================================================

String gerarPaginaFavoritas() {

  String html;
  html.reserve(12000);

  html += "<!DOCTYPE html>";
  html += "<html lang='pt-BR'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Escolha suas Favoritas</title>";

  html += "<style>";
  html += "body{background:#111;color:#fff;font-family:Arial;margin:0;padding:20px;}";
  html += ".container{max-width:700px;margin:auto;}";
  html += "h1{color:#FFD700;text-align:center;}";
  html += ".instrucao{text-align:center;}";
  html += "#contador{text-align:center;font-size:20px;font-weight:bold;color:#FFD700;margin:20px;}";
  html += ".radio{display:flex;align-items:center;padding:12px;margin:8px 0;";
  html += "background:#1d1d1d;border:1px solid #444;border-radius:8px;}";
  html += ".radio input{width:22px;height:22px;margin-right:12px;}";
  html += ".ordem{display:inline-block;min-width:28px;color:#FFD700;font-weight:bold;}";
  html += "button{width:100%;padding:15px;background:#FFD700;color:#111;";
  html += "font-size:18px;font-weight:bold;border:0;border-radius:8px;}";
  html += "button:disabled{background:#555;color:#aaa;}";
  html += "</style>";

  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";

  html += "<h1>CENTRAL RÁDIOS BRASIL</h1>";
  html += "<p class='instrucao'>Escolha exatamente 5 rádios favoritas.</p>";
  html += "<p class='instrucao'>A ordem em que marcar será a ordem das posições 1 a 5.</p>";

  html += "<div id='contador'>Selecionadas: 0/5</div>";

  html += "<form method='POST' action='/salvar' onsubmit='return prepararEnvio();'>";

  for (int i = 0; i < totalRadios; i++) {

    html += "<label class='radio'>";

html += "<input type='checkbox' class='seletora' value='";
html += String(i);
html += "' onchange='alterarSelecao(this)'";

bool jaFavorita = false;

for (int f = 0; f < quantidadeFavoritas; f++) {
  if (favoritas[f] == i) {
    jaFavorita = true;
    break;
  }
}

if (jaFavorita) {
  html += " checked";
}

html += ">";

    html += "<span class='ordem' id='ordem";
    html += String(i);
    html += "'></span>";

    html += radios[i].nome;

    if (radios[i].cidade.length() > 0) {
      html += " - ";
      html += radios[i].cidade;
    }

    if (radios[i].estado.length() > 0) {
      html += "/";
      html += radios[i].estado;
    }

    html += "</label>";
  }

  for (int i = 0; i < MAX_FAVORITAS; i++) {
    html += "<input type='hidden' name='fav";
    html += String(i);
    html += "' id='fav";
    html += String(i);
    html += "'>";
  }

  html += "<button id='salvar' type='submit' disabled>Salvar favoritas</button>";
  html += "<button id='salvar' type='submit' disabled>Salvar favoritas</button>";
  html += "</form>";
  html += "</div>";

  html += "<script>";
  html += "let escolhidas=[];";

  html += "function alterarSelecao(c){";
  html += "let valor=c.value;";

  html += "if(c.checked){";
  html += "if(escolhidas.length>=5){";
  html += "c.checked=false;";
  html += "alert('Você só pode escolher 5 rádios.');";
  html += "return;";
  html += "}";
  html += "escolhidas.push(valor);";
  html += "}else{";
  html += "escolhidas=escolhidas.filter(v=>v!==valor);";
  html += "}";

  html += "atualizarTela();";
  html += "}";

  html += "function atualizarTela(){";
  html += "document.querySelectorAll('.ordem').forEach(e=>e.textContent='');";

  html += "escolhidas.forEach((valor,posicao)=>{";
  html += "document.getElementById('ordem'+valor).textContent=(posicao+1)+'.';";
  html += "});";

  html += "document.getElementById('contador').textContent=";
  html += "'Selecionadas: '+escolhidas.length+'/5';";

  html += "document.getElementById('salvar').disabled=(escolhidas.length!==5);";
  html += "}";

  html += "function prepararEnvio(){";

  html += "if(escolhidas.length!==5){";
  html += "alert('Escolha exatamente 5 rádios.');";
  html += "return false;";
  html += "}";

  html += "for(let i=0;i<5;i++){";
  html += "document.getElementById('fav'+i).value=escolhidas[i];";
  html += "}";

  html += "return true;";
  html += "}";

  html += "</script>";
  html += "</body>";
  html += "</html>";

  return html;
}
//======================================================
// RECEBER E GRAVAR AS 5 FAVORITAS
//======================================================

void receberFavoritas() {

  int escolhas[MAX_FAVORITAS];

  for (int i = 0; i < MAX_FAVORITAS; i++) {

    String campo = "fav" + String(i);

    if (!servidor.hasArg(campo)) {
      servidor.send(
        400,
        "text/plain; charset=utf-8",
        "Selecione exatamente 5 rádios."
      );
      return;
    }

    escolhas[i] = servidor.arg(campo).toInt();

    if (escolhas[i] < 0 || escolhas[i] >= totalRadios) {
      servidor.send(
        400,
        "text/plain; charset=utf-8",
        "Uma das rádios selecionadas é inválida."
      );
      return;
    }

    for (int j = 0; j < i; j++) {
      if (escolhas[i] == escolhas[j]) {
        servidor.send(
          400,
          "text/plain; charset=utf-8",
          "Não é permitido escolher a mesma rádio duas vezes."
        );
        return;
      }
    }
  }

  for (int i = 0; i < MAX_FAVORITAS; i++) {

    favoritas[i] = escolhas[i];

    memoria.putString(
        ("favId" + String(i)).c_str(),
        radios[escolhas[i]].id
    );

}

  quantidadeFavoritas = MAX_FAVORITAS;
  assistenteInicialConcluido = true;

  salvarFavoritas();
  memoria.putBool("assistenteOK", true);

  servidor.send(
    200,
    "text/html; charset=utf-8",
    "<!DOCTYPE html>"
    "<html lang='pt-BR'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<meta http-equiv='refresh' content='3;url=/'>"
    "</head>"
    "<body style='background:#111;color:#fff;font-family:Arial;text-align:center;padding:30px'>"
"<h1 style='color:#FFD700'>Favoritas salvas com sucesso!</h1>"
"<p>As novas favoritas foram gravadas no aparelho.</p>"
    "</body>"
    "</html>"
  );


}
//======================================================
// INICIAR SERVIDOR DAS FAVORITAS
//======================================================

void iniciarServidorFavoritas() {
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP("CENTRAL_RADIOS_BRASIL");

  IPAddress ipPortal = WiFi.softAPIP();

  servidorDNS.start(
    53,
    "*",
    ipPortal
  );

  Serial.println();
  Serial.println("Portal cativo das favoritas iniciado.");
  Serial.print("Rede Wi-Fi: CENTRAL_DAS_RADIOS");
  Serial.print(" - Endereco: http://");
  Serial.println(ipPortal);
servidor.on("/favoritos", HTTP_GET, []() {

    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );

  });
  // Android
  servidor.on("/generate_204", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );
  });

  servidor.on("/gen_204", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );
  });

  // iPhone e iPad
  servidor.on("/hotspot-detect.html", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );
  });

  servidor.on("/library/test/success.html", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );
  });

  // Windows
  servidor.on("/ncsi.txt", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );
  });

  servidor.on("/connecttest.txt", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );
  });

  servidor.on("/fwlink", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );
  });
  servidor.on("/salvar", HTTP_POST, receberFavoritas);

servidor.onNotFound([]() {

    servidor.send(
        404,
        "text/plain",
        "Pagina nao encontrada"
    );

});

  servidor.begin();

  Serial.println();
  Serial.println("========================================");
  Serial.println("SERVIDOR DE FAVORITAS INICIADO");
  Serial.print("Endereco: http://");
  Serial.println(WiFi.localIP());
  Serial.println("========================================");
}
//======================================================
// ORGANIZAR RÁDIOS: FAVORITAS PRIMEIRO
//======================================================

void organizarRadiosComFavoritas() {

  if (totalRadios <= 1) {
    return;
  }

  // Primeiro coloca todas as rádios em ordem alfabética.
  for (int i = 0; i < totalRadios - 1; i++) {

    for (int j = i + 1; j < totalRadios; j++) {

      String nomeA = radios[i].nome;
      String nomeB = radios[j].nome;

      nomeA.toLowerCase();
      nomeB.toLowerCase();

      if (nomeA.compareTo(nomeB) > 0) {

        Radio temporaria = radios[i];
        radios[i] = radios[j];
        radios[j] = temporaria;
      }
    }
  }

  // No primeiro uso, ainda não existem favoritas.
  // Portanto, mantém apenas a ordem alfabética.
  if (!assistenteInicialConcluido) {
    return;
  }

  // Move cada favorita para sua posição:
  // favorita 1 = posição 0, favorita 2 = posição 1 etc.
  for (int posicaoFavorita = 0;
       posicaoFavorita < MAX_FAVORITAS;
       posicaoFavorita++) {

    String idFavorita = memoria.getString(
      ("favId" + String(posicaoFavorita)).c_str(),
      ""
    );

    if (idFavorita.length() == 0) {
      continue;
    }

    int indiceEncontrado = -1;

    for (int i = posicaoFavorita; i < totalRadios; i++) {

      if (radios[i].id == idFavorita) {
        indiceEncontrado = i;
        break;
      }
    }

    if (indiceEncontrado < 0) {
      continue;
    }

    // Guarda a favorita encontrada.
    Radio radioFavorita = radios[indiceEncontrado];

    // Desloca as outras rádios uma posição para a direita.
    // Assim, as demais continuam em ordem alfabética.
    for (int i = indiceEncontrado;
         i > posicaoFavorita;
         i--) {

      radios[i] = radios[i - 1];
    }

    // Coloca a favorita na posição correta.
    radios[posicaoFavorita] = radioFavorita;
  }

  // Atualiza os índices numéricos das favoritas.
  quantidadeFavoritas = MAX_FAVORITAS;
  salvarFavoritas();

  for (int i = 0; i < MAX_FAVORITAS; i++) {
    favoritas[i] = i;
  }

  salvarFavoritas();

  Serial.println();
  Serial.println("ORDEM FINAL DAS RADIOS:");

  for (int i = 0; i < totalRadios; i++) {
    Serial.print(i + 1);
    Serial.print(" - ");
    Serial.println(radios[i].nome);
  }
}
void atualizarListaFavoritos() {

  organizarRadiosComFavoritas();

  if (estacao >= totalRadios)
    estacao = 0;

  conectaRadio();

}
//======================================================
// PAINEL CENTRAL - FASE 1
//======================================================

String gerarPaginaCentral() {
  String html;
  html.reserve(8000);

  String nomeRadio = "Nenhuma rádio";

  if (totalRadios > 0 &&
      estacao >= 0 &&
      estacao < totalRadios) {
    nomeRadio = radios[estacao].nome;
  }



  html += F("<!DOCTYPE html>");
  html += F("<html lang='pt-BR'>");
  html += F("<head>");
  html += F("<meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");

  html += F("<style>");
  html += F("*{box-sizing:border-box}");

html += F("body{margin:0;padding:22px 14px;background:");
html += F("radial-gradient(circle at top,#123b91 0,#061a4b 38%,#020b22 78%);");
html += F("min-height:100vh;font-family:Arial,Helvetica,sans-serif;color:#fff}");

html += F(".painel{width:100%;max-width:580px;margin:0 auto}");

html += F(".titulo{text-align:center;margin-bottom:26px}");
html += F(".titulo h1{font-size:32px;margin:10px 0;color:#ffe13e;letter-spacing:1px}");
html += F(".titulo p{margin:0;color:#d7e3ff;font-size:14px}");

html += F(".cartao{background:rgba(255,255,255,.97);color:#07163d;");
html += F("border-radius:24px;padding:26px 22px;margin-bottom:20px;");
html += F("border:1px solid rgba(255,255,255,.45);");
html += F("box-shadow:0 14px 36px rgba(0,0,0,.34)}");
html += F(".radio-atual{text-align:center}");
html += F(".radio-atual small{display:block;color:#68748e;margin-bottom:10px;font-size:17px;font-weight:700}");
html += F(".radio-atual strong{display:block;font-size:25px;color:#07163d;line-height:1.3}");

html += F(".linha{display:flex;align-items:center;justify-content:space-between;");
html += F(".rotulo{color:#66718a;font-size:17px;font-weight:700}");
html += F("background:#f5f7fb;border:1px solid #e2e7f0;border-radius:12px}");

html += F(".rotulo{color:#66718a;font-size:14px;font-weight:600}");

html += F(".valor{font-weight:800;text-align:right;color:#07163d;");
html += F("max-width:62%;word-break:break-word}");
html += F("input[type='range']{width:100%;height:12px;cursor:pointer;");
html += F("accent-color:#f4b900;margin-top:12px}");
html += F("#valorVolume{display:block;text-align:center;font-size:22px;");
html += F("font-weight:900;color:#07163d;margin-top:6px}");
html += F(".status-ok{color:#14843c;background:#e7f7ec;");
html += F("padding:5px 9px;border-radius:20px;font-size:13px}");

html += F(".status-erro{color:#bd2020;background:#fdeaea;");
html += F("padding:5px 9px;border-radius:20px;font-size:13px}");

  html += F(".menu{display:grid;grid-template-columns:1fr 1fr;gap:12px}");

html += F(".botao{width:100%;display:flex;justify-content:center;align-items:center;");
html += F("min-height:92px;padding:18px 14px;");
html += F("border-radius:18px;background:linear-gradient(145deg,#ffe44d,#f5b400);");
html += F("color:#061331;text-decoration:none;font-size:18px;font-weight:800;");
html += F("text-align:center;border:1px solid rgba(255,255,255,.60);");
html += F("box-shadow:0 10px 22px rgba(0,0,0,.22);");
html += F("transition:transform .15s ease,box-shadow .15s ease}");

html += F(".botao:hover{transform:translateY(-2px);");
html += F("box-shadow:0 10px 20px rgba(0,0,0,.24)}");

html += F(".botao:active{transform:scale(.97)}");

html += F(".desativado{opacity:.48;pointer-events:none;filter:grayscale(.35)}");

html += F(".rodape{text-align:center;color:#b8c8f6;");
html += F("font-size:11px;margin-top:20px;letter-spacing:.4px}");

  html += F("@media(max-width:370px){");
  html += F(".menu{grid-template-columns:1fr}");
  
  html += F("}");
  html += F("</style>");

  html += F("</head>");
  html += F("<body>");
  

  html += F("<main class='painel'>");

html += F("<header class='titulo'>");
html += F("<h1>CENTRAL RÁDIOS BRASIL</h1>");
html += F("</header>");

html += F("<section class='cartao radio-atual'>");
html += F("<small>🎵 Rádio Atual</small>");
html += F("<strong id='radioAtual' style='display:block;text-align:center;font-size:22px;margin-top:8px'>");
html += nomeRadio;
html += F("</strong>");
html += F("</section>");

  html += F("<section class='cartao'>");

  

  html += F("<div class='linha'>");
 html += F("<div class='linha' style='display:block'>");

html += F("<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px'>");
html += F("<span class='rotulo'>🔊 Volume</span>");
html += F("<span id='valorVolume' class='valor'>");
html += String(volumeAtual);
html += F("%</span>");
html += F("</div>");

html += F("<input id='controleVolume' type='range' min='0' max='100' value='");
html += String(volumeAtual);
html += F("' style='width:100%'>");

html += F("</div>");
html += F("<div class='linha' style='display:block'>");

html += F("<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px'>");

html += F("<button id='btnAnterior' type='button' class='botao'>");
html += F("◀ Anterior");
html += F("</button>");

html += F("<button id='btnProxima' type='button' class='botao'>");
html += F("Próxima ▶");
html += F("</button>");

html += F("</div>");
html += F("</div>");
  
  html += F("</section>");

html += F("<section class='cartao'>");

html += F("<div style='display:grid;gap:12px'>");

html += F("<button id='btnFavoritar' class='botao' type='button'>");
html += F("⭐ Favoritar");
html += F("</button>");

html += F("<a class='botao' href='/wifi'>");
html += F("📶 Alterar Wi-Fi");
html += F("</a>");

html += F("<button id='btnReiniciar' class='botao' type='button'>");
html += F("🔄 Reiniciar");
html += F("</button>");

html += F("</div>");

html += F("</section>");

  html += F("<div class='rodape'>");

html += F("<div>CENTRAL RÁDIOS BRASIL</div>");

html += F("<div>Firmware ");
html += VERSAO_FIRMWARE;
html += F("</div>");

html += F("</div>");

 html += F("<script>");

html += F("async function atualizarPainel(){");
html += F("try{");

html += F("const r=await fetch('/status?t='+Date.now(),{cache:'no-store'});");
html += F("if(!r.ok)return;");

html += F("const d=await r.json();");
html += F("const favorito=document.getElementById('btnFavoritar');");

html += F("if(favorito){");
html += F("favorito.textContent=d.favorita?");
html += F("'★ Remover dos favoritos':");
html += F("'☆ Adicionar aos favoritos';");
html += F("}");

html += F("document.getElementById('radioAtual').textContent=d.radio;");


html += F("}catch(e){}");
html += F("}");
html += F("const controleVolume=document.getElementById('controleVolume');");
html += F("const valorVolume=document.getElementById('valorVolume');");
html += F("const btnAnterior=document.getElementById('btnAnterior');");
html += F("const btnProxima=document.getElementById('btnProxima');");

html += F("btnAnterior.addEventListener('click',async()=>{");
html += F("btnAnterior.disabled=true;");
html += F("try{");
html += F("await fetch('/anterior',{method:'POST'});");
html += F("setTimeout(atualizarPainel,700);");
html += F("}catch(e){}");
html += F("setTimeout(()=>{btnAnterior.disabled=false;},900);");
html += F("});");

html += F("btnProxima.addEventListener('click',async()=>{");
html += F("btnProxima.disabled=true;");
html += F("try{");
html += F("await fetch('/proxima',{method:'POST'});");
html += F("setTimeout(atualizarPainel,700);");
html += F("}catch(e){}");
html += F("setTimeout(()=>{btnProxima.disabled=false;},900);");
html += F("});");
html += F("let atrasoVolume;");

html += F("controleVolume.addEventListener('input',function(){");
html += F("valorVolume.textContent=this.value+'%';");
html += F("clearTimeout(atrasoVolume);");

html += F("atrasoVolume=setTimeout(async()=>{");
html += F("try{");
html += F("const dados=new URLSearchParams();");
html += F("dados.append('valor',controleVolume.value);");

html += F("await fetch('/volume',{");
html += F("method:'POST',");
html += F("headers:{'Content-Type':'application/x-www-form-urlencoded'},");
html += F("body:dados.toString()");
html += F("});");

html += F("}catch(e){}");
html += F("},250);");
html += F("});");
html += F("const btnFavoritar=document.getElementById('btnFavoritar');");
html += F("const btnReiniciar=document.getElementById('btnReiniciar');");

html += F("btnFavoritar.addEventListener('click',async()=>{");
html += F("btnFavoritar.disabled=true;");

html += F("try{");

html += F("const r=await fetch('/favoritar-atual',{method:'POST'});");
html += F("const j=await r.json();");

html += F("if(j.favorita){");
html += F("btnFavoritar.textContent='⭐ Remover dos favoritos';");
html += F("}else{");
html += F("btnFavoritar.textContent='⭐ Favoritar';");
html += F("}");

html += F("}catch(e){");
html += F("alert('Erro ao alterar favorito.');");
html += F("}");

html += F("setTimeout(()=>{btnFavoritar.disabled=false;},500);");
html += F("});");
html += F("btnReiniciar.addEventListener('click',async()=>{");
html += F("if(!confirm('Deseja reiniciar o aparelho?'))return;");
html += F("btnReiniciar.disabled=true;");
html += F("try{");
html += F("await fetch('/reiniciar',{method:'POST'});");
html += F("btnReiniciar.textContent='Reiniciando...';");
html += F("}catch(e){");
html += F("btnReiniciar.disabled=false;");
html += F("}");
html += F("});");
html += F("atualizarPainel();");
html += F("setInterval(atualizarPainel,2000);");

html += F("</script>");
  html += F("</main>");
  html += F("</body>");
  html += F("</html>");

  return html;
}
void iniciarPainelCentral() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Painel central nao iniciado: Wi-Fi desconectado.");
    return;
  }

  if (!MDNS.begin("central")) {
    Serial.println("Falha ao iniciar o endereco central.local.");
  } else {
    MDNS.addService("http", "tcp", 80);

    Serial.println();
    Serial.println("========================================");
    Serial.println("PAINEL CENTRAL INICIADO");
    Serial.println("Endereco: http://central.local");
    Serial.print("Endereco alternativo: http://");
    Serial.println(WiFi.localIP());
    Serial.println("========================================");
  }




servidor.on("/", HTTP_GET, []() {
  servidor.send(
    200,
    "text/html; charset=utf-8",
    gerarPaginaCentral()
  );
});
servidor.on("/status", HTTP_GET, []() {

  String radioAtual = "Nenhuma rádio";
  if (
    totalRadios > 0 &&
    estacao >= 0 &&
    estacao < totalRadios
  ) {
    radioAtual = radios[estacao].nome;
  }

  bool wifiConectado =
    WiFi.status() == WL_CONNECTED;

  bool radioConectada =
    radioClient != nullptr &&
    radioClient->connected();
bool favoritaAtual = false;

if (
  totalRadios > 0 &&
  estacao >= 0 &&
  estacao < totalRadios
) {
  favoritaAtual = radioEhFavorita(estacao);
}
  DynamicJsonDocument doc(512);

  doc["radio"] = radioAtual;
  doc["favorita"] = favoritaAtual;

  doc["wifi"] = wifiConectado
    ? "Conectado"
    : "Desconectado";

  doc["radioStatus"] = radioConectada
    ? "Conectada"
    : "Desconectada";

  doc["tempo"] =
    (millis() - inicioBoot) / 1000UL;

  doc["sinal"] = wifiConectado
    ? String(WiFi.RSSI()) + " dBm"
    : "Sem sinal";

  doc["memoria"] =
    String(ESP.getFreeHeap() / 1024) + " KB";

  String resposta;
  serializeJson(doc, resposta);

  servidor.send(
    200,
    "application/json; charset=utf-8",
    resposta
  );
});
servidor.on("/volume", HTTP_POST, []() {

  if (!servidor.hasArg("valor")) {
    servidor.send(
      400,
      "application/json; charset=utf-8",
      "{\"erro\":\"Valor do volume não informado\"}"
    );
    return;
  }

  int novoVolume = servidor.arg("valor").toInt();

  if (novoVolume < 0) {
    novoVolume = 0;
  }

  if (novoVolume > 100) {
    novoVolume = 100;
  }

  volumeAtual = (uint8_t)novoVolume;

  player.setVolume(volumeAtual);
  salvarVolume();

  String resposta = "{\"volume\":";
  resposta += String(volumeAtual);
  resposta += "}";

  servidor.send(
    200,
    "application/json; charset=utf-8",
    resposta
  );
});
servidor.on("/favoritar-atual", HTTP_POST, []() {

  if (
    totalRadios <= 0 ||
    estacao < 0 ||
    estacao >= totalRadios
  ) {
    servidor.send(
      400,
      "application/json; charset=utf-8",
      "{\"erro\":\"Nenhuma rádio válida selecionada\"}"
    );
    return;
  }

  bool agoraFavorita = alternarFavoritaAtual();

  String resposta = "{\"favorita\":";
  resposta += agoraFavorita ? "true" : "false";
  resposta += ",\"quantidade\":";
  resposta += String(quantidadeFavoritas);
  resposta += "}";

  servidor.send(
    200,
    "application/json; charset=utf-8",
    resposta
  );
});
  servidor.on("/favoritas", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaFavoritas()
    );
  });

  servidor.on("/salvar", HTTP_POST, receberFavoritas);
    servidor.on("/wifi", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaWiFi()
    );
  });
    servidor.on("/info", HTTP_GET, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      gerarPaginaInfo()
    );
  });
  servidor.on("/trocar-wifi", HTTP_POST, []() {
    servidor.send(
      200,
      "text/html; charset=utf-8",
      "<!DOCTYPE html>"
      "<html lang='pt-BR'>"
      "<head>"
      "<meta charset='UTF-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "</head>"
      "<body style='background:#111;color:#fff;font-family:Arial;text-align:center;padding:30px'>"
      "<h1 style='color:#FFD700'>Alterando Wi-Fi</h1>"
      "<p>A rede atual foi removida.</p>"
      "<p>O aparelho será reiniciado e abrirá a rede CENTRAL_DAS_RADIOS.</p>"
      "</body>"
      "</html>"
    );

    delay(1000);

    WiFiManager wm;
    wm.resetSettings();

    delay(1000);
    ESP.restart();
  });
  servidor.on("/proxima", HTTP_POST, []() {

  nextEstacao();

  servidor.send(
    200,
    "application/json; charset=utf-8",
    "{\"ok\":true}"
  );

});

servidor.on("/anterior", HTTP_POST, []() {

  prevEstacao();

  servidor.send(
    200,
    "application/json; charset=utf-8",
    "{\"ok\":true}"
  );

});
servidor.on("/reiniciar", HTTP_POST, []() {
  servidor.send(200, "text/plain", "Reiniciando");
  delay(500);
  ESP.restart();
});
  servidor.onNotFound([]() {
  servidor.send(
    404,
    "text/plain; charset=utf-8",
    "Pagina nao encontrada"
  );
});

  servidor.begin();
}



String gerarPaginaWiFi() {

  String html;

  html += "<!DOCTYPE html>";
  html += "<html lang='pt-BR'>";

  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Configuração Wi-Fi</title>";
  html += "</head>";

  html += "<body style='background:#111;color:#fff;font-family:Arial;text-align:center;padding:30px'>";

  html += "<h1 style='color:#FFD700'>Configuração Wi-Fi</h1>";

if (WiFi.status() == WL_CONNECTED) {

  html += "<p><strong>Estado:</strong> Conectado</p>";

  html += "<p><strong>Rede atual:</strong> ";
  html += WiFi.SSID();
  html += "</p>";

  html += "<p><strong>Endereço IP:</strong> ";
  html += WiFi.localIP().toString();
  html += "</p>";

  html += "<p><strong>Intensidade do sinal:</strong> ";
  html += String(WiFi.RSSI());
  html += " dBm</p>";

} else {

  html += "<p><strong>Estado:</strong> Desconectado</p>";

}
html += "<p style='margin-top:25px;color:#ccc'>Ao continuar, o aparelho esquecerá a rede atual e abrirá novamente o modo de configuração.</p>";

html += "<form method='POST' action='/trocar-wifi'>";

html += "<button type='submit' style='padding:14px 22px;background:#FFD700;color:#111;border:0;border-radius:8px;font-size:16px;font-weight:bold;cursor:pointer'>Trocar rede Wi-Fi</button>";

html += "</form>";

  html += "<br>";

  html += "<a href='/' style='display:inline-block;padding:12px 22px;background:#444;color:#fff;text-decoration:none;border-radius:8px'>← Voltar ao Painel</a>";

  html += "</body>";
  html += "</html>";

  return html;
}
String gerarPaginaInfo() {

  String html;

  html += "<!DOCTYPE html>";
  html += "<html lang='pt-BR'>";

  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Informações do Aparelho</title>";
  html += "</head>";

  html += "<body style='background:#111;color:#fff;font-family:Arial;text-align:center;padding:30px'>";

  html += "<h1 style='color:#FFD700'>Informações do Aparelho</h1>";

  html += "<p>As informações técnicas da CENTRAL RÁDIOS BRASIL serão exibidas nesta página.</p>";
html += "<hr>";

html += "<p><strong>Versão:</strong> 2.0</p>";

html += "<p><strong>Firmware:</strong> CENTRAL RÁDIOS BRASIL</p>";

html += "<p><strong>IP:</strong> ";
html += WiFi.localIP().toString();
html += "</p>";

html += "<p><strong>Rede Wi-Fi:</strong> ";
html += WiFi.SSID();
html += "</p>";

html += "<p><strong>Sinal:</strong> ";
html += String(WiFi.RSSI());
html += " dBm</p>";

html += "<p><strong>Memória Flash:</strong> ";
html += String(ESP.getFlashChipSize() / 1024 / 1024);
html += " MB</p>";

html += "<p><strong>Heap Livre:</strong> ";
html += String(ESP.getFreeHeap() / 1024);
html += " KB</p>";

html += "<p><strong>Uptime:</strong> ";
html += String(millis()/1000);
html += " segundos</p>";
 html += "<hr>";

html += "<h2 style='color:#FFD700'>Hardware</h2>";

html += "<p><strong>Chip:</strong> ESP32</p>";

html += "<p><strong>Núcleos:</strong> ";
html += String(ESP.getChipCores());
html += "</p>";

html += "<p><strong>Frequência CPU:</strong> ";
html += String(getCpuFrequencyMhz());
html += " MHz</p>";

html += "<p><strong>Flash Livre:</strong> ";
html += String(ESP.getFreeSketchSpace() / 1024);
html += " KB</p>";

html += "<p><strong>Tamanho do Programa:</strong> ";
html += String(ESP.getSketchSize() / 1024);
html += " KB</p>";

html += "<p><strong>SDK:</strong> ";
html += ESP.getSdkVersion();
html += "</p>";
  html += "<br>";
html += "<style>";
html += "body{background:#111;color:#fff;font-family:Arial;margin:0;padding:20px;text-align:center;}";
html += ".painel{max-width:650px;margin:auto;}";
html += ".cartao{background:#1d1d1d;border:1px solid #333;border-radius:12px;padding:15px;margin:12px 0;}";
html += ".cartao p{margin:10px 0;}";
html += "h1,h2{color:#FFD700;}";
html += "</style>";
  html += "<a href='/' style='display:inline-block;padding:12px 22px;background:#444;color:#fff;text-decoration:none;border-radius:8px'>← Voltar ao Painel</a>";

  html += "</body>";
  html += "</html>";

  return html;
}