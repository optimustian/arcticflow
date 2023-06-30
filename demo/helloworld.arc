flow add(i8 *a, i8 *b, i8 *c) {
    *c = *a + *b;
}

flow main() {
    i8 a[100], b[100], c[100];
    
    for (i8 i = 0; i < 100; i++) {
        a[i] = i;
        b[i] = i;
    }
    for(i8 i = 0; i < 100; i++) {
        add(a + i, b + i, c + i);
    }
}