# 🚀 EmbarcaTech

Este repositório reúne as **atividades** e **projetos** desenvolvidos durante a residência **EmbarcaTech** em sistemas embarcados.  
O foco principal é o aprendizado e a prática com a plataforma **Raspberry Pi Pico**.  

---

## 📂 Estrutura do Repositório

As atividades estão organizadas em **unidades** dentro da pasta `Atividades` e podem ser executadas diretamente no ambiente de desenvolvimento utilizando a extensão **Raspberry Pi Pico Project**.  

🔹 **Exemplo de caminho:**  
```
Atividades/Unidade_1/Aplicação_multitarefa_multtask
```  

> ⚠️ **Observação:** Todos os projetos utilizam o **Pico SDK v1.5.1**.  

---

## 🔧 Sensores e Atuadores

A pasta **Sensores_e_Atuadores** contém as bibliotecas para os sensores e atuadores utilizados nos projetos.  
Todas seguem um padrão para facilitar a reutilização e integração:  

### 📌 Estrutura de cada biblioteca
- 📁 **lib/** → Arquivos `.c` e `.h` principais da biblioteca.  
- 📝 **Exemplo de Uso** → Código de demonstração no arquivo principal.  
- 📖 **README dedicado** com:  
  - Como utilizar a biblioteca nos projetos  
  - Configuração do **CMakeLists.txt**  
  - Exemplos práticos de implementação  

🔹 **Exemplo de lib:**  
Sensor de luminosidade BH1750
caminho:
Sensores_Atuadores/luminos_BH1750
```  
luminos_BH1750/
├── lib/                   # Arquivos da biblioteca que será ultilizado em outros projetos
│   ├── bh1750.c
│   └── bh1750.h
├── CMakeList.txt           # CMake configurado para o projeto
├── luminos_BH1750.c        # Arquivos de exemplo de uso, como uma demostração de como usar a lib
└──  README.md              # Todo tutorial de uso da lib e principais funções.
```
> ⚠️ **Observação:** Nos projetos que você irá ultilizar as lib, basta apenas criar uma pasta lib na raiz, e copiar e colar as bibliotecas apresentadas nesse material, posteriormente configurar o cmake e importar o arquivos corretamente para o arquivo que executará as libs.  

---

## ⚙️ Instruções de Configuração

1. **Instale o Pico SDK v1.5.1** para garantir compatibilidade.  
2. **Configure o ambiente de desenvolvimento** utilizando **CMake**.  
   - Cada biblioteca possui instruções próprias no README.  
3. **Execute os exemplos fornecidos** para validar o funcionamento e aprender a integração.  

---

💡 Essa estrutura foi pensada para ser clara, modular e escalável, permitindo evolução contínua dos projetos durante o programa EmbarcaTech.  
