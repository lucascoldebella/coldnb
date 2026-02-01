/*
 * Admin Password Hash Generator
 *
 * Compile: gcc -o generate_admin_password generate_admin_password.c -lsodium
 * Usage:   ./generate_admin_password <password>
 *
 * Outputs an Argon2id hash suitable for inserting into admin_users table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <password>\n", argv[0]);
        fprintf(stderr, "\nGenerates an Argon2id hash for the admin_users table.\n");
        return 1;
    }

    if (sodium_init() < 0) {
        fprintf(stderr, "Error: Failed to initialize libsodium\n");
        return 1;
    }

    const char *password = argv[1];
    size_t password_len = strlen(password);

    if (password_len < 8) {
        fprintf(stderr, "Error: Password must be at least 8 characters\n");
        return 1;
    }

    if (password_len > 128) {
        fprintf(stderr, "Error: Password must be at most 128 characters\n");
        return 1;
    }

    /* Generate hash using Argon2id */
    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
            hash,
            password,
            password_len,
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
        fprintf(stderr, "Error: Failed to hash password (out of memory?)\n");
        return 1;
    }

    printf("\nGenerated Argon2id hash:\n");
    printf("%s\n\n", hash);

    printf("SQL to update admin password:\n");
    printf("UPDATE admin_users SET password_hash = '%s' WHERE username = 'admin';\n\n", hash);

    printf("Or for INSERT:\n");
    printf("INSERT INTO admin_users (username, email, password_hash, full_name, role)\n");
    printf("VALUES ('admin', 'admin@yourdomain.com', '%s', 'Administrator', 'super_admin');\n\n", hash);

    /* Clear password from memory */
    sodium_memzero((void *)password, password_len);

    return 0;
}
