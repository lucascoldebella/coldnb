import { NextResponse } from "next/server";
import { writeFile, mkdir } from "fs/promises";
import { existsSync } from "fs";
import path from "path";

const UPLOAD_DIR = process.env.UPLOAD_DIR
  ? path.join(process.env.UPLOAD_DIR, "products")
  : path.join(process.cwd(), "public", "uploads", "products");
const MAX_FILE_SIZE = 5 * 1024 * 1024; // 5MB

export async function POST(request) {
  try {
    const formData = await request.formData();
    const files = formData.getAll("files");

    if (!files || files.length === 0) {
      return NextResponse.json(
        { success: false, error: "No files provided" },
        { status: 400 }
      );
    }

    if (!existsSync(UPLOAD_DIR)) {
      await mkdir(UPLOAD_DIR, { recursive: true });
    }

    const uploadedUrls = [];
    const errors = [];

    for (const file of files) {
      if (!file || typeof file === "string") continue;

      // Validate file type
      const validTypes = ["image/jpeg", "image/png", "image/gif", "image/webp"];
      if (!validTypes.includes(file.type)) {
        errors.push(`${file.name}: unsupported file type`);
        continue;
      }

      // Validate file size
      if (file.size > MAX_FILE_SIZE) {
        errors.push(`${file.name}: exceeds 5MB limit`);
        continue;
      }

      const timestamp = Date.now();
      const randomStr = Math.random().toString(36).substring(2, 8);
      const ext = file.name.split(".").pop();
      const filename = `${timestamp}-${randomStr}.${ext}`;

      const bytes = await file.arrayBuffer();
      const buffer = Buffer.from(bytes);

      const filePath = path.join(UPLOAD_DIR, filename);
      await writeFile(filePath, buffer);

      uploadedUrls.push(`/uploads/products/${filename}`);
    }

    if (uploadedUrls.length === 0) {
      return NextResponse.json(
        { success: false, error: errors.length > 0 ? errors.join("; ") : "No valid images uploaded" },
        { status: 400 }
      );
    }

    return NextResponse.json({
      success: true,
      data: {
        urls: uploadedUrls,
      },
    });
  } catch (_error) {
    return NextResponse.json(
      { success: false, error: "Failed to upload files" },
      { status: 500 }
    );
  }
}
