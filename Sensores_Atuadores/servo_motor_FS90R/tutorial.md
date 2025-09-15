# ⚡ Boas práticas
O FS90R é um servo de rotação contínua barato, e girar sempre na velocidade máxima (PWM 1000 µs ou 2000 µs) pode ter alguns efeitos negativos a longo prazo

* Evitar manter o servo sempre no extremo (1000 ou 2000 µs).

* Se possível, usar valores intermediários (1100–1900 µs) para reduzir desgaste e calor.

* Garantir boa alimentação (3.3 V está ok, mas se perceber fraco, um conversor 5 V estável é melhor).

* Permitir pausas ou mudanças de direção para não ficar rodando sempre no mesmo sentido.