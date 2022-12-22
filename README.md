# RC_Word_Game

Projeto da cadeira de Redes de Computadores de LEIC-A. (Ano 2022/2023)

## Objetivo:
O objetivo deste projeto é desenvolver um jogo de palavras simples, inspirado no clássico jogo da forca.


## Ficheiro Makefile e como Compilar:
- make - Para compilar o projeto basta executar o comando make dentro da diretoria /RC_Word_Game. Este comando irá criar dois executáveis (um para o Server dentro da diretoria /GS, e outro para o player dentro da diretoria /client).
- make clean - O comando make clean irá apagar todos os executáveis criados pelo comando make, e ainda os ficheiros resultantes da execução do jogo (ficheiros recebidos pelos comandos hint, scoreboard e state do lado do client; ficheiros de jogos começados e possivelmente acabados e scores do lado do server).


## Como executar:
- Player: dentro da diretoria raiz (/RC_Word_Game) executar o comando ./client/player [-n GSIP] [-p GSport]. Os dois argumentos são opcionais, e correspondem ao IP onde é corrido o server e ao port do mesmo, respetivamente.
- Server: **dentro da diretoria raiz (RC_Word_Game, é importante se seja executado dentro desta)** executar o comando ./GS/server word_eng.txt [-p GSport] [-v]. O primeiro argumento corresponde ao ficheiro com as palavras dos jogos situado em /RC_Word_Game/GS/Dados_GS. O segundo elemento e o terceiro elemento são opcionais, correspondendo ao port do server e à opção de verbose (server vai dando print do que acontece).

**ATENÇÃO!** Foram retirados os ficheiros correspondentes às imagens do comando _hint_ por simplificação. Antes de executar o programa, os ficheiros devem ser colocados dentro da diretoria /RC_Word_Game/GS/Dados_GS . É importante que estes sejam colocados na diretoria em questão para o funcionamento do programa.


## Comandos:
- start _PLID_ ou sg _PLID_ - começa um novo jogo do jogador _PLID_.
- play _letter_ ou pl _letter_ - nova jogada com a letra _letter_.
- guess _word_ ou gw _word_ - jogada onde tenta adivinhar a palavra inteira.
- scoreboard ou sb - visualiza e guarda num ficheiro os Top 10 Scores entre todos os jogadores.
- hint ou h - pedido de uma dica para ajudar a encotrar a palavra desconhecida (recebe um ficheiro)-
- state ou st - visualiza e guarda num ficheiro o estado do jogo atual, ou a evolução do último jogo do jogador caso este não esteja com nunhum jogo ativo.
- quit - desiste do jogo atual.
- exit - fechar o programa.


## Pasta reports:
A pasta reports contém o resultado da execução dos testes para o Server fornecidos pelos professores.


## TIMEOUT - Como retirar o timeout dos sockets:
Foi adicionado um timeout aos sockets do player para que, em caso de erro, o cliente não se mantivesse à espera de resposta infinitamente. O timout colocado foi de 15 segundos, mas este é facilmente alterável no ficheiro /RC_Word_Game/client/constants.h . Para alterar o timeout basta substituir o valor da constante TIMEOUT pelo desejado (ou, em caso de se querer remover, colocar o valor desta constante a 0).
