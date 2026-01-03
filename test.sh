#!/bin/bash
# Script de test pour mysh

echo "=== Test de compilation ==="
make clean
make

if [ $? -ne 0 ]; then
    echo "Erreur de compilation!"
    exit 1
fi

echo ""
echo "=== Compilation réussie ==="
echo ""
echo "Exécutables créés:"
ls -lh mysh myls myps

echo ""
echo "=== Test de myls ==="
./myls -a /tmp | head -n 10

echo ""
echo "=== Test de myps ==="
./myps | head -n 10

echo ""
echo "=== Instructions pour tester mysh ==="
echo "Lancez ./mysh et testez les commandes suivantes:"
echo ""
echo "1. Commandes simples:"
echo "   ls -la"
echo "   pwd"
echo "   echo Hello World"
echo ""
echo "2. Séquencement:"
echo "   ls ; pwd"
echo "   mkdir test && cd test"
echo "   cd nonexistent || echo 'Directory not found'"
echo ""
echo "3. Wildcards:"
echo "   ls *.c"
echo "   echo /etc/????"
echo ""
echo "4. Redirections:"
echo "   ls > output.txt"
echo "   cat < output.txt"
echo "   ls | grep mysh"
echo ""
echo "5. Background jobs:"
echo "   sleep 10 &"
echo "   myjobs"
echo "   myfg 1"
echo ""
echo "6. Variables:"
echo "   set test=hello"
echo "   echo \$test"
echo "   setenv MYVAR=world"
echo "   echo \$MYVAR"
echo ""
echo "7. Commandes internes:"
echo "   cd ~"
echo "   status"
echo "   exit"
echo ""
echo "Pour lancer le shell: ./mysh"
