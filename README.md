# Prédicteurs de Branchement

Ce dépôt contient **uniquement** les fichiers modifiés dans le cadre d'un TP sur les prédicteurs de branchement réalisés à Ensimag.

## Description

Les fichiers inclus :
- `predictor.cc` : Implémentation des prédicteurs.
- `predictor.h` : Définitions des structures et interfaces associées.

Ces fichiers font partie d'un projet plus large qui n'est pas inclus ici pour respecter les droits d'auteur et simplifier le partage.

## Prédicteurs Implémentés

### 1. **Prédicteur GShare**
   - Utilise un historique global combiné à l'adresse du branchement via un XOR.
   - Réduit les conflits d'adresses en utilisant un masque dynamique sur les bits significatifs.
   - Simplifie la prédiction pour des branches répétitives.


### 2. **Prédicteur Local**
   - S'appuie sur une table des historiques locaux indexée par les bits faibles de l'adresse du branchement.
   - Chaque entrée de la table des historiques correspond à un compteur bimodal de 2 bits pour prendre des décisions basées sur des branches locales.


### 3. **Métapréducteur**
   - Combine deux prédicteurs : un prédicteur global GShare et un prédicteur local (2-level).
   - Utilise un métapréducteur pour décider lequel des deux prédicteurs est le plus fiable pour une branche donnée.
   - Ajoute un mécanisme d’apprentissage basé sur des compteurs pour améliorer la précision au fil du temps.


### 4. **Prédicteur Perceptron**
   - Implémente un prédicteur basé sur un réseau de neurones simplifié (perceptron).
   - Utilise un historique global comme vecteur d'entrée et effectue un produit scalaire avec des poids associés.
   - Améliore la précision pour des branches complexes avec des motifs non linéaires.


## Limitations

Ce dépôt **ne contient pas** les dépendances nécessaires pour exécuter le code. Les fichiers présentés sont à titre de démonstration uniquement.

## Contexte

Le TP portait sur l'implémentation et l'évaluation de différents algorithmes pour prédire les branchements dans des architectures de processeurs.

### Objectifs :
- Comparer la précision de différents prédicteurs.
- Comprendre les compromis entre espace mémoire, temps d'exécution et précision.
- Explorer l'utilisation de techniques avancées comme les perceptrons dans un contexte d'architecture de processeurs.