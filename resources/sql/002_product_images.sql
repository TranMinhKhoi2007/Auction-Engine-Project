CREATE TABLE product_images (
    product_id INTEGER PRIMARY KEY REFERENCES products(id),
    data BLOB NOT NULL CHECK(length(data) BETWEEN 1 AND 262144)
);
