#include <efi.h>
#include <efilib.h>

#define MAX_PASSWORD_LEN 32
#define NEXT_BOOTLOADER L"\\EFI\\BOOT\\bootloader2.efi"

/* ---------------------------------------------------------------------
 * Minimal, self-contained SHA-256 implementation (no external deps).
 * Public-domain style implementation, adapted for UEFI/gnu-efi types.
 * ------------------------------------------------------------------- */

typedef struct {
  UINT8  data[64];
  UINT32 datalen;
  UINT64 bitlen;
  UINT32 state[8];
} SHA256_CTX;

#define ROTR(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SHA256_CH(x,y,z)  (((x) & (y)) ^ (~(x) & (z)))
#define SHA256_MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SHA256_EP0(x) (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define SHA256_EP1(x) (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define SHA256_SIG0(x) (ROTR(x,7) ^ ROTR(x,18) ^ ((x) >> 3))
#define SHA256_SIG1(x) (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10))

static const UINT32 sha256_k[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const UINT8 data[])
{
  UINT32 a, b, c, d, e, f, g, h, t1, t2, m[64];
  UINTN i, j;

  for (i = 0, j = 0; i < 16; ++i, j += 4)
    m[i] = (data[j] << 24) | (data[j+1] << 16) | (data[j+2] << 8) | (data[j+3]);
  for (; i < 64; ++i)
    m[i] = SHA256_SIG1(m[i-2]) + m[i-7] + SHA256_SIG0(m[i-15]) + m[i-16];

  a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
  e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

  for (i = 0; i < 64; ++i) {
    t1 = h + SHA256_EP1(e) + SHA256_CH(e,f,g) + sha256_k[i] + m[i];
    t2 = SHA256_EP0(a) + SHA256_MAJ(a,b,c);
    h = g; g = f; f = e; e = d + t1;
    d = c; c = b; b = a; a = t1 + t2;
  }

  ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
  ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx)
{
  ctx->datalen = 0;
  ctx->bitlen = 0;
  ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
  ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
  ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
  ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const UINT8 data[], UINTN len)
{
  UINTN i;
  for (i = 0; i < len; ++i) {
    ctx->data[ctx->datalen] = data[i];
    ctx->datalen++;
    if (ctx->datalen == 64) {
      sha256_transform(ctx, ctx->data);
      ctx->bitlen += 512;
      ctx->datalen = 0;
    }
  }
}

static void sha256_final(SHA256_CTX *ctx, UINT8 hash[32])
{
  UINTN i = ctx->datalen;

  if (ctx->datalen < 56) {
    ctx->data[i++] = 0x80;
    while (i < 56) ctx->data[i++] = 0x00;
  } else {
    ctx->data[i++] = 0x80;
    while (i < 64) ctx->data[i++] = 0x00;
    sha256_transform(ctx, ctx->data);
    ZeroMem(ctx->data, 56);
  }

  ctx->bitlen += ((UINT64)ctx->datalen) * 8;
  ctx->data[63] = (UINT8)(ctx->bitlen);
  ctx->data[62] = (UINT8)(ctx->bitlen >> 8);
  ctx->data[61] = (UINT8)(ctx->bitlen >> 16);
  ctx->data[60] = (UINT8)(ctx->bitlen >> 24);
  ctx->data[59] = (UINT8)(ctx->bitlen >> 32);
  ctx->data[58] = (UINT8)(ctx->bitlen >> 40);
  ctx->data[57] = (UINT8)(ctx->bitlen >> 48);
  ctx->data[56] = (UINT8)(ctx->bitlen >> 56);
  sha256_transform(ctx, ctx->data);

  for (i = 0; i < 4; ++i) {
    UINTN j;
    for (j = 0; j < 8; ++j) {
      hash[j * 4 + i] = (UINT8)(ctx->state[j] >> (24 - i * 8));
    }
  }
}

/* ---------------------------------------------------------------------
 * Expected SHA-256 hash of the correct password, encoded as UTF-16LE
 * bytes (matching CHAR16 in-memory representation), WITHOUT the null
 * terminator.
 *
 * Generate this with the included `generate_hash.py` helper script:
 *     python3 generate_hash.py "your-password-here"
 *
 * The placeholder below corresponds to the password "admin1".
 * ------------------------------------------------------------------- */
static const UINT8 EXPECTED_HASH[32] = {
  0x39, 0xcf, 0x77, 0xa7, 0x6c, 0x36, 0xca, 0x8a,
  0xdd, 0x99, 0xa7, 0xe4, 0xd5, 0xaa, 0x92, 0x9a,
  0x9a, 0x7d, 0x5f, 0x0a, 0xde, 0xad, 0x0f, 0xbf,
  0x4c, 0x6c, 0x2b, 0xa7, 0x62, 0xb9, 0x8c, 0x2f,
};

static BOOLEAN HashMatches(CHAR16 *Input, UINTN InputLen)
{
  SHA256_CTX ctx;
  UINT8 digest[32];
  UINTN i;

  sha256_init(&ctx);
  sha256_update(&ctx, (UINT8 *)Input, InputLen * sizeof(CHAR16));
  sha256_final(&ctx, digest);

  for (i = 0; i < 32; ++i) {
    if (digest[i] != EXPECTED_HASH[i]) {
      return FALSE;
    }
  }
  return TRUE;
}

/* ----------------------------------------------------------------- */

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;
  CHAR16 InputBuffer[MAX_PASSWORD_LEN + 1];
  UINTN InputLen;

  InitializeLib(ImageHandle, SystemTable);

  SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut = SystemTable->ConOut;
  SIMPLE_INPUT_INTERFACE *ConIn = SystemTable->ConIn;

  /* Reset console to a known-good text mode before doing anything else.
   * Helps on older/quirky firmware that leaves ConOut in a bad state
   * after the boot manager hands off control. */
  uefi_call_wrapper(ConOut->Reset, 2, ConOut, FALSE);
  uefi_call_wrapper(ConOut->SetMode, 2, ConOut, 0);

  uefi_call_wrapper(ConOut->ClearScreen, 1, ConOut);
  uefi_call_wrapper(ConOut->SetAttribute, 2, ConOut, EFI_WHITE);
  uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"Enter password: ");

  InputLen = 0;
  InputBuffer[0] = 0;

  while (TRUE) {
    UINTN Index;
    uefi_call_wrapper(SystemTable->BootServices->WaitForEvent, 3, 1, &ConIn->WaitForKey, &Index);
    Status = uefi_call_wrapper(ConIn->ReadKeyStroke, 2, ConIn, &Key);
    if (EFI_ERROR(Status)) {
      continue;
    }

    if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
      break;
    }
    else if (Key.UnicodeChar == CHAR_BACKSPACE) {
      if (InputLen > 0) {
        InputLen--;
        InputBuffer[InputLen] = 0;
        uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"\b \b");
      }
    }
    else if (Key.UnicodeChar != 0 && InputLen < MAX_PASSWORD_LEN) {
      InputBuffer[InputLen] = Key.UnicodeChar;
      InputLen++;
      InputBuffer[InputLen] = 0;
      uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"*");
    }
  }

  uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"\r\n");

  if (HashMatches(InputBuffer, InputLen)) {
    uefi_call_wrapper(ConOut->SetAttribute, 2, ConOut, EFI_GREEN);
    uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"Password correct.\r\n");
    uefi_call_wrapper(ConOut->SetAttribute, 2, ConOut, EFI_WHITE);
    uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"Booting in 2 seconds...\r\n");
    uefi_call_wrapper(SystemTable->BootServices->Stall, 1, 2 * 1000 * 1000);

    EFI_HANDLE NextImageHandle;
    EFI_LOADED_IMAGE *LoadedImage;
    EFI_DEVICE_PATH *DevicePath;

    Status = uefi_call_wrapper(
      SystemTable->BootServices->HandleProtocol, 3,
      ImageHandle, &LoadedImageProtocol, (void **)&LoadedImage
    );
    if (EFI_ERROR(Status)) {
      uefi_call_wrapper(ConOut->SetAttribute, 2, ConOut, EFI_RED);
      uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"Error: cannot get loaded image protocol.\r\n");
      return Status;
    }

    DevicePath = FileDevicePath(LoadedImage->DeviceHandle, NEXT_BOOTLOADER);

    Status = uefi_call_wrapper(
      SystemTable->BootServices->LoadImage, 6,
      FALSE, ImageHandle, DevicePath, NULL, 0, &NextImageHandle
    );
    if (EFI_ERROR(Status)) {
      uefi_call_wrapper(ConOut->SetAttribute, 2, ConOut, EFI_RED);
      uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"Error: failed to load next bootloader.\r\n");
      return Status;
    }

    Status = uefi_call_wrapper(
      SystemTable->BootServices->StartImage, 3,
      NextImageHandle, NULL, NULL
    );
    return Status;
  }
  else {
    uefi_call_wrapper(ConOut->SetAttribute, 2, ConOut, EFI_RED);
    uefi_call_wrapper(ConOut->OutputString, 2, ConOut, L"Incorrect password.\r\n");
    uefi_call_wrapper(ConOut->SetAttribute, 2, ConOut, EFI_WHITE);

    uefi_call_wrapper(SystemTable->ConIn->Reset, 2, SystemTable->ConIn, FALSE);
    while (uefi_call_wrapper(SystemTable->ConIn->ReadKeyStroke, 2, SystemTable->ConIn, &Key) == EFI_NOT_READY);

    return EFI_ACCESS_DENIED;
  }
}
