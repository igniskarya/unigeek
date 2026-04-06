const Footer = () => {
  return (
    <footer>
      <div className="soc">
        <a
          target="_blank"
          rel="noreferrer"
          href="https://github.com/lshaf/unigeek"
        >
          <span className="ion ion-social-github" />
        </a>
      </div>
      <div className="copy">
        &copy; {new Date().getFullYear()} UniGeek. Open-source firmware.
      </div>
      <div className="clr" />
    </footer>
  );
};

export default Footer;