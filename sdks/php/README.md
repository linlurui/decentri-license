# PHP SDK 使用示例

## 安装

1. 将 `decentrilicense.php` 文件复制到您的项目目录中
2. 在您的 PHP 文件中引入该文件：

```php
require_once 'decentrilicense.php';
```

## 基本用法

### 验证许可证令牌

```php
<?php
require_once 'decentrilicense.php';

// 创建 DecentriLicense 实例
$dl = new DecentriLicense();

// 加载根公钥（用于验证许可证公钥签名）
$rootPublicKey = "-----BEGIN PUBLIC KEY-----
这里放置您的根公钥
-----END PUBLIC KEY-----";

// 加载令牌文件
$tokenJson = file_get_contents('path/to/token.json');

// 验证令牌
$result = $dl->verifyToken($tokenJson, $rootPublicKey);

if ($result['valid']) {
    echo "令牌验证成功！\n";
    echo "应用ID: " . $result['app_id'] . "\n";
    echo "许可证代码: " . $result['license_code'] . "\n";
    echo "过期时间: " . $result['expire_time'] . "\n";
} else {
    echo "令牌验证失败: " . $result['error'] . "\n";
}
?>
```

## API 参考

### DecentriLicense 类

#### verifyToken($tokenJson, $rootPublicKey)
验证许可证令牌

参数:
- `$tokenJson` (string): 令牌JSON字符串
- `$rootPublicKey` (string): 根公钥PEM格式字符串

返回:
- `array`: 包含验证结果的关联数组
  - `valid` (bool): 验证是否成功
  - `app_id` (string): 应用ID（仅在验证成功时存在）
  - `license_code` (string): 许可证代码（仅在验证成功时存在）
  - `expire_time` (string): 过期时间（仅在验证成功时存在）
  - `error` (string): 错误信息（仅在验证失败时存在）

## 注意事项

1. 确保您的PHP环境支持OpenSSL扩展
2. 根公钥必须与签发令牌时使用的根私钥匹配
3. 令牌文件必须是有效的JSON格式