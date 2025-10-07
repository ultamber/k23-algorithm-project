# Vector Search Project

## Περιγραφή
Υλοποίηση αλγορίθμων αναζήτησης διανυσμάτων:
- LSH
- Hypercube
- IVFFlat
- IVFPQ

## Οδηγίες Μεταγλώττισης
```bash
make
./search -d <dataset file> -q <query file> -N 10 -R 2000 -method lsh